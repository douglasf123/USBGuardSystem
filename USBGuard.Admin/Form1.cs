using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace USBGuard.Admin
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
            ListarUnidades();
        }

        // Preenche o ComboBox 'cboUnidades' com as letras dos drives (E:\, F:\, etc)
        private void ListarUnidades()
        {
            try
            {
                cboUnidades.Items.Clear();
                foreach (var drive in DriveInfo.GetDrives())
                {
                    if (drive.IsReady)
                    {
                        cboUnidades.Items.Add(drive.Name);
                    }
                }

                if (cboUnidades.Items.Count > 0)
                    cboUnidades.SelectedIndex = 0;
            }
            catch (Exception ex)
            {
                MessageBox.Show("Erro ao listar unidades: " + ex.Message);
            }
        }

        // Evento do botão "Detectar/Ler Serial"
        private void btnLerSerial_Click(object sender, EventArgs e)
        {
            if (cboUnidades.SelectedItem == null)
            {
                MessageBox.Show("Selecione uma unidade na lista!");
                return;
            }

            string letraSelecionada = cboUnidades.SelectedItem.ToString();

            // Usa o UsbDetector para pegar o serial tratado (aquele do diagnóstico)
            string serialLimpo = UsbDetector.GetPhysicalSerialNumber(letraSelecionada);

            // Se a solução nativa obteve um serial válido, usa-o
            bool nativeOk = !(serialLimpo.IndexOf("Erro", StringComparison.OrdinalIgnoreCase) >= 0 || serialLimpo.IndexOf("não encontrado", StringComparison.OrdinalIgnoreCase) >= 0);
            if (nativeOk)
            {
                txtSerial.Text = serialLimpo;
                txtSerial.ReadOnly = true;
                MessageBox.Show("Serial extraído com sucesso!");
                return;
            }

            // Caso contrário, tenta extrair do PNPDeviceID via WMI como fallback local
            try
            {
                string root = letraSelecionada.Trim();
                if (root.EndsWith("\\")) root = root.Substring(0, root.Length - 1);
                string found = null;
                using (var searcher = new System.Management.ManagementObjectSearcher("SELECT * FROM Win32_DiskDrive"))
                {
                    foreach (System.Management.ManagementObject disk in searcher.Get())
                    {
                        string deviceId = disk["DeviceID"]?.ToString();
                        using (var parts = new System.Management.ManagementObjectSearcher($"ASSOCIATORS OF {{Win32_DiskDrive.DeviceID='{deviceId}'}} WHERE AssocClass = Win32_DiskDriveToDiskPartition"))
                        {
                            foreach (System.Management.ManagementObject part in parts.Get())
                            {
                                using (var logs = new System.Management.ManagementObjectSearcher($"ASSOCIATORS OF {{Win32_DiskPartition.DeviceID='{part["DeviceID"]}'}} WHERE AssocClass = Win32_LogicalDiskToPartition"))
                                {
                                    foreach (var ld in logs.Get())
                                    {
                                        if (string.Equals(ld["DeviceID"]?.ToString(), root, StringComparison.OrdinalIgnoreCase))
                                        {
                                            found = disk["PNPDeviceID"]?.ToString();
                                            break;
                                        }
                                    }
                                }
                                if (found != null) break;
                            }
                        }
                        if (found != null) break;
                    }
                }

                if (found != null)
                {
                    int lastSlash = found.LastIndexOf('\\');
                    string candidate = lastSlash >= 0 && lastSlash + 1 < found.Length ? found.Substring(lastSlash + 1) : found;
                    int amp = candidate.IndexOf('&');
                    if (amp >= 0) candidate = candidate.Substring(0, amp);
                    candidate = candidate.Trim();

                    // popula o campo de serial com o candidato extraído
                    txtSerial.Text = candidate;
                    txtSerial.ReadOnly = true;
                    MessageBox.Show($"Serial extraído do PNPDeviceID: {candidate}", "Sucesso (fallback)", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    // ambas falharam: informa o usuário
                    MessageBox.Show(serialLimpo, "Falha na Leitura", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    txtSerial.Text = "";
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Erro diagnóstico PNP: " + ex.Message);
                MessageBox.Show(serialLimpo, "Falha na Leitura", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                txtSerial.Text = "";
            }
        }

        // Evento do botão "Gerar .AUTH" (btnGerar)
        private void btnGerar_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrWhiteSpace(txtAppID.Text) || string.IsNullOrWhiteSpace(txtSerial.Text))
            {
                MessageBox.Show("O AppID e o Serial são obrigatórios!");
                return;
            }

            try
            {
                // Preenche a struct USBAuthTicket com arrays de tamanho fixo (ASCII)
                USBAuthTicket ticket = new USBAuthTicket();
                ticket.Magic = 0x53425355;

                // AppID -> 64 bytes ASCII
                ticket.AppID = new byte[64];
                byte[] appBytes = Encoding.ASCII.GetBytes(txtAppID.Text.Trim());
                Array.Clear(ticket.AppID, 0, ticket.AppID.Length);
                Array.Copy(appBytes, 0, ticket.AppID, 0, Math.Min(appBytes.Length, ticket.AppID.Length));

                // Serial -> 256 bytes ASCII
                ticket.Serial = new byte[256];
                byte[] serialBytes = Encoding.ASCII.GetBytes(txtSerial.Text.Trim());
                Array.Clear(ticket.Serial, 0, ticket.Serial.Length);
                Array.Copy(serialBytes, 0, ticket.Serial, 0, Math.Min(serialBytes.Length, ticket.Serial.Length));

                ticket.Reserved = new byte[188];

                // Gravar diretamente no disco selecionado
                if (cboUnidades.SelectedItem == null)
                {
                    MessageBox.Show("Selecione uma unidade na lista para gravar o arquivo.", "Erro", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }

                string targetDrive = cboUnidades.SelectedItem.ToString(); // ex: "E:\\"
                string targetPath = Path.Combine(targetDrive, "device.auth");

                // Converte para binário (512 bytes) manualmente para garantir layout exato
                const int totalSize = 512;
                byte[] buffer = new byte[totalSize];

                // Magic (4 bytes, little-endian)
                byte[] magicBytes = BitConverter.GetBytes(ticket.Magic);
                Array.Copy(magicBytes, 0, buffer, 0, Math.Min(magicBytes.Length, 4));

                // AppID at offset 4 (64 bytes)
                Array.Copy(ticket.AppID, 0, buffer, 4, Math.Min(ticket.AppID.Length, 64));

                // Serial at offset 68 (4 + 64)
                Array.Copy(ticket.Serial, 0, buffer, 68, Math.Min(ticket.Serial.Length, 256));

                // Reserved at offset 324 (4 + 64 + 256)
                // We'll place a SHA-256 over the first 324 bytes into the first 32 bytes of Reserved
                Array.Copy(ticket.Reserved, 0, buffer, 324, Math.Min(ticket.Reserved.Length, 188));

                // Compute SHA-256 and store in Reserved[0..31]
                using (var sha = System.Security.Cryptography.SHA256.Create())
                {
                    byte[] hash = sha.ComputeHash(buffer, 0, 324);
                    Array.Copy(hash, 0, buffer, 324, Math.Min(hash.Length, 188));
                }

                // Se já existir, confirma sobrescrita
                if (File.Exists(targetPath))
                {
                    var resp = MessageBox.Show($"O arquivo '{targetPath}' já existe. Deseja sobrescrever?", "Confirmar sobrescrita", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                    if (resp != DialogResult.Yes) return;
                }

                File.WriteAllBytes(targetPath, buffer);

                // Marca o arquivo como oculto
                try
                {
                    File.SetAttributes(targetPath, File.GetAttributes(targetPath) | FileAttributes.Hidden);
                }
                catch
                {
                    // Se falhar em setar atributo, não bloqueia o fluxo principal
                }

                MessageBox.Show($"Arquivo 'device.auth' gerado com sucesso em {targetPath}", "Sucesso", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Erro ao gerar arquivo: " + ex.Message);
            }
        }
    }
}