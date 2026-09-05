param([string]$Executable = (Join-Path $PSScriptRoot '..\build\windows\Release\RegShare.exe'))
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Windows.Forms,System.Drawing -TypeDefinition @'
using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;

public static class RegionShareSmoke {
    [StructLayout(LayoutKind.Sequential)] struct Rect { public int L,T,R,B; }
    [StructLayout(LayoutKind.Sequential)] struct Point { public int X,Y; }
    [DllImport("user32.dll")] static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern IntPtr FindWindowEx(IntPtr parent, IntPtr after, string cls, string title);
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr hwnd, IntPtr after, int x,int y,int w,int h,uint flags);
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr hwnd, out Rect rect);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr hwnd, out Rect rect);
    [DllImport("user32.dll")] static extern int GetWindowLong(IntPtr hwnd,int index);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr hwnd, ref Point point);
    [DllImport("user32.dll")] static extern bool GetWindowDisplayAffinity(IntPtr hwnd, out uint affinity);
    [DllImport("user32.dll")] static extern uint GetDpiForWindow(IntPtr hwnd);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr hwnd,uint msg,IntPtr wp,IntPtr lp);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr hwnd,int show);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr hwnd,StringBuilder text,int max);
    static readonly Color[] Colors = { Color.FromArgb(220,40,50), Color.FromArgb(25,170,100),
        Color.FromArgb(30,100,220), Color.FromArgb(230,190,40) };
    static void Pump(int milliseconds) {
        var clock = Stopwatch.StartNew();
        while(clock.ElapsedMilliseconds < milliseconds) { Application.DoEvents(); Thread.Sleep(10); }
    }
    static void Check(bool condition,string message) { if(!condition) throw new Exception(message); }
    static void Status(IntPtr output,bool visible,string contains) {
        var label = FindWindowEx(output,IntPtr.Zero,"STATIC",null);
        var text = new StringBuilder(1024); GetWindowText(label,text,text.Capacity);
        Check(IsWindowVisible(label)==visible && (!visible || text.ToString().Contains(contains)),
            "Unexpected capture status: " + text);
    }
    static void Snapshot(IntPtr output,string path,int sourceWidth,int sourceHeight,int[] expected) {
        Rect r; GetClientRect(output,out r); Point p = new Point(); ClientToScreen(output,ref p);
        using(var bitmap = new Bitmap(r.R,r.B)) {
            using(var graphics = Graphics.FromImage(bitmap))
                graphics.CopyFromScreen(p.X,p.Y,0,0,new Size(r.R,r.B));
            bitmap.Save(path,ImageFormat.Png);
            float scale = Math.Min((float)r.R/sourceWidth,(float)r.B/sourceHeight);
            float w=sourceWidth*scale,h=sourceHeight*scale;
            for(int i=0;i<4;i++) {
                var actual=bitmap.GetPixel((int)((r.R-w)/2+w*(i%2==0?.25:.75)),
                    (int)((r.B-h)/2+h*(i<2?.25:.75)));
                var wanted=Colors[expected[i]];
                Check(Math.Abs(actual.R-wanted.R)<20 && Math.Abs(actual.G-wanted.G)<20 &&
                    Math.Abs(actual.B-wanted.B)<20,"Pixel mismatch in " + path + ": " + actual + " expected " + wanted);
            }
        }
    }
    public static void Run(string executable,string artifacts) {
        SetProcessDpiAwarenessContext(new IntPtr(-4));
        Check(FindWindow("RegShareWindowClass",null)==IntPtr.Zero,"Close existing RegionShare before this test.");
        var area=Screen.PrimaryScreen.WorkingArea;
        Check(area.Width>=1280 && area.Height>=600,"Smoke test needs a 1280 x 600 desktop or larger.");
        int x=area.Left+40,y=area.Top+80;
        using(var source = new Form()) {
            source.FormBorderStyle=FormBorderStyle.None;
            source.StartPosition=FormStartPosition.Manual;
            source.Bounds=new Rectangle(x,y,640,360);
            source.ShowInTaskbar=false;
            int colorShift=0;
            source.Paint += (sender,args) => {
                for(int i=0;i<4;i++) using(var brush=new SolidBrush(Colors[(i+colorShift)%4]))
                    args.Graphics.FillRectangle(brush,(i%2)*320,(i/2)*180,320,180);
            };
            source.Show(); Pump(300);
            var process=Process.Start(executable);
            try {
                Pump(3000);
                var output=FindWindow("RegShareWindowClass",null);
                var selector=FindWindow("RegionShareSelector",null);
                Check(output!=IntPtr.Zero && selector!=IntPtr.Zero,"Application windows did not start.");
                SetWindowPos(output,new IntPtr(-1),x+700,y,500,400,0x10);
                int dpi=(int)GetDpiForWindow(selector),b=(5*dpi+48)/96,t=(28*dpi+48)/96;
                SetWindowPos(selector,IntPtr.Zero,x-b,y-t,640+2*b,360+t+b,0x14);
                Pump(3000);
                uint affinity;
                Check(GetWindowDisplayAffinity(selector,out affinity) && affinity==0x11,"Selector is not excluded.");
                Check(GetWindowDisplayAffinity(output,out affinity) && affinity==0,"Output is excluded from capture.");
                Status(output,false,"");
                Snapshot(output,System.IO.Path.Combine(artifacts,"live.png"),640,360,new[]{0,1,2,3});
                colorShift=1; source.Invalidate(); Pump(1000);
                Snapshot(output,System.IO.Path.Combine(artifacts,"updated.png"),640,360,new[]{1,2,3,0});
                colorShift=0; source.Invalidate(); Pump(1000);
                SetWindowPos(selector,IntPtr.Zero,x+320-b,y-t,320+2*b,360+t+b,0x14);
                Pump(1000);
                Snapshot(output,System.IO.Path.Combine(artifacts,"recrop.png"),320,360,new[]{1,1,3,3});
                SendMessage(output,0x111,new IntPtr(100),IntPtr.Zero); Pump(300);
                Status(output,true,"Paused");
                SendMessage(output,0x111,new IntPtr(100),IntPtr.Zero); Pump(1500);
                Status(output,false,"");
                SetWindowPos(output,IntPtr.Zero,x+350,y+20,500,400,0x14); Pump(500);
                Status(output,true,"overlaps");
                SetWindowPos(output,IntPtr.Zero,x+700,y,500,400,0x14); Pump(1500);
                Status(output,false,"");
                ShowWindow(output,6); Pump(300);
                Check(!IsWindowVisible(selector),"Minimize did not hide selector.");
                ShowWindow(output,9); Pump(1500);
                Check(IsWindowVisible(selector),"Restore did not show selector.");
                Status(output,false,"");
                Snapshot(output,System.IO.Path.Combine(artifacts,"restored.png"),320,360,new[]{1,1,3,3});
                SendMessage(output,0x111,new IntPtr(102),IntPtr.Zero);
                Check((GetWindowLong(output,-20)&8)!=0,"Always-on-top toggle failed.");
                SendMessage(output,0x111,new IntPtr(102),IntPtr.Zero);
                Check((GetWindowLong(output,-20)&8)==0,"Always-on-top reset failed.");
                SendMessage(output,0x111,new IntPtr(103),IntPtr.Zero);
                SendMessage(output,0x111,new IntPtr(104),IntPtr.Zero); Pump(1000);
                Status(output,false,"");
                Rect preset;
                SendMessage(output,0x111,new IntPtr(105),IntPtr.Zero); Pump(200);
                GetWindowRect(selector,out preset);
                Check(preset.R-preset.L-2*b==1280 && preset.B-preset.T-t-b==720,"720p preset dimensions.");
                SendMessage(output,0x111,new IntPtr(106),IntPtr.Zero); Pump(200);
                GetWindowRect(selector,out preset);
                Check(preset.R-preset.L-2*b==1920 && preset.B-preset.T-t-b==1080,"1080p preset dimensions.");
                Console.WriteLine("PASS: live and updated GPU capture pixels, static recrop, exclusion flags, pause/resume, overlap recovery, minimize/restore, topmost, cursor/monitor toggles, fixed presets. DPI="+dpi);
            } finally {
                if(!process.HasExited) { process.CloseMainWindow(); Pump(500); }
                if(!process.HasExited) process.Kill();
                process.Dispose();
                source.Close();
            }
        }
    }
}
'@
$artifacts = Join-Path $PSScriptRoot '..\build\smoke'
New-Item -ItemType Directory -Force -Path $artifacts | Out-Null
[RegionShareSmoke]::Run((Resolve-Path $Executable).ProviderPath, (Resolve-Path $artifacts).ProviderPath)
