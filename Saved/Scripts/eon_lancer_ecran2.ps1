# EON — lance l'éditeur (ou le jeu) avec des arguments, place ses fenêtres sur l'écran 2 (X=-1920, Y=506) sans voler
# le focus, attend la fin. Usage : powershell -File eon_lancer_ecran2.ps1 -Script <chemin.py> [-Log nom.log] [-Max 900]
# (le script python part par -ExecCmds="py ..." : l'éditeur reste ouvert jusqu'à ce que le script le ferme).
param([string]$Script, [string]$Ligne, [string]$Log = "EON_ecran2.log", [int]$Max = 900)
# -Ligne : ligne de commande complète (ex. jeu en -game) ; appeler alors ce script par & dans PowerShell, pas par -File
# (powershell -File retire les guillemets intérieurs).
if (-not $Ligne) { $Ligne = "`"D:/_PROJETS/briscard/EON_UE/EON.uproject`" -ExecCmds=`"py $Script`" -nosplash -log=$Log" }
Add-Type @"
using System; using System.Runtime.InteropServices; using System.Text;
public class W {
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc f, IntPtr p);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@
$exe = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$p = Start-Process -FilePath $exe -ArgumentList $Ligne -PassThru
if (-not $p) { "ECHEC du lancement"; exit 1 }
$t0 = Get-Date
while (-not $p.HasExited -and ((Get-Date) - $t0).TotalSeconds -lt $Max) {
  [W]::EnumWindows({ param($h, $x)
    $id = 0; [void][W]::GetWindowThreadProcessId($h, [ref]$id)
    if ($id -eq $p.Id -and [W]::IsWindowVisible($h)) {
      $r = New-Object W+RECT; [void][W]::GetWindowRect($h, [ref]$r)
      if ($r.L -ge 0 -and ($r.R - $r.L) -gt 300) { [void][W]::SetWindowPos($h, [IntPtr]::Zero, -1920, 506, 0, 0, 0x0001 -bor 0x0004 -bor 0x0010) }
    }
    return $true }, [IntPtr]::Zero) | Out-Null
  Start-Sleep -Milliseconds 300
}
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; "TEMPS DEPASSE, arrete" } else { "fini, code $($p.ExitCode)" }
