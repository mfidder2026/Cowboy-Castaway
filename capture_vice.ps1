Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

Add-Type -ReferencedAssemblies System.Drawing @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;
public class WinCapture {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }

    public static Bitmap Capture(IntPtr hWnd) {
        RECT rc;
        GetWindowRect(hWnd, out rc);
        int w = rc.Right - rc.Left;
        int h = rc.Bottom - rc.Top;
        Bitmap bmp = new Bitmap(w, h);
        using (Graphics g = Graphics.FromImage(bmp)) {
            IntPtr hdc = g.GetHdc();
            PrintWindow(hWnd, hdc, 0);
            g.ReleaseHdc(hdc);
        }
        return bmp;
    }
}
"@

$p = Get-Process x64sc | Select-Object -First 1
if (!$p) { Write-Host 'no x64sc process'; exit 1 }
$h = $p.MainWindowHandle
if ($h -eq 0) { Write-Host 'no main window handle'; exit 1 }

$bmp = [WinCapture]::Capture($h)
$bmp.Save('D:\dev\cc65\vice_window.png')
$bmp.Dispose()
Write-Host "captured"
