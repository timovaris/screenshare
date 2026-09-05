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
using System.Threading;
using System.Windows.Forms;

public static class RegionShareScreenshots {
    [DllImport("user32.dll")] static extern bool SetProcessDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern IntPtr FindWindow(string cls,string title);
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr hwnd,IntPtr after,int x,int y,int w,int h,uint flags);
    [DllImport("user32.dll")] static extern uint GetDpiForWindow(IntPtr hwnd);
    [DllImport("user32.dll")] static extern bool PostMessage(IntPtr hwnd,uint msg,IntPtr wp,IntPtr lp);
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr hwnd);
    static void Pump(int duration) {
        var timer=Stopwatch.StartNew();
        while(timer.ElapsedMilliseconds<duration) { Application.DoEvents(); Thread.Sleep(10); }
    }
    static void Text(Graphics g,string text,int size,FontStyle style,Color color,int x,int y) {
        using(var font=new Font("Segoe UI",size,style,GraphicsUnit.Pixel))
        using(var brush=new SolidBrush(color)) g.DrawString(text,font,brush,x,y);
    }
    static void Save(Rectangle bounds,string path) {
        using(var bitmap=new Bitmap(bounds.Width,bounds.Height)) {
            using(var g=Graphics.FromImage(bitmap))
                g.CopyFromScreen(bounds.Location,System.Drawing.Point.Empty,bounds.Size);
            bitmap.Save(path,ImageFormat.Png);
        }
    }
    public static void Run(string executable,string directory) {
        SetProcessDpiAwarenessContext(new IntPtr(-4));
        if(FindWindow("RegShareWindowClass",null)!=IntPtr.Zero)
            throw new Exception("Close RegionShare before taking documentation screenshots.");
        var area=Screen.PrimaryScreen.WorkingArea;
        if(area.Width<1540 || area.Height<720) throw new Exception("A 1540 x 720 desktop is required.");
        int x=area.Left+20,y=area.Top+20;
        using(var backdrop=new Form()) using(var source=new Form()) {
            backdrop.FormBorderStyle=FormBorderStyle.None;
            backdrop.StartPosition=FormStartPosition.Manual;
            backdrop.Bounds=new Rectangle(x,y,1500,660);
            backdrop.BackColor=Color.FromArgb(235,240,239);
            backdrop.ShowInTaskbar=false;
            backdrop.Paint+=(sender,e)=> {
                Text(e.Graphics,"SELECTED REGION",13,FontStyle.Bold,Color.FromArgb(55,87,77),42,30);
                Text(e.Graphics,"SHAREABLE OUTPUT",13,FontStyle.Bold,Color.FromArgb(55,87,77),838,30);
            };
            source.FormBorderStyle=FormBorderStyle.None;
            source.StartPosition=FormStartPosition.Manual;
            source.Bounds=new Rectangle(x+48,y+116,720,405);
            source.BackColor=Color.White;
            source.ShowInTaskbar=false;
            source.Paint+=(sender,e)=> {
                var g=e.Graphics;
                Text(g,"TEAM WORKSPACE  /  PLANNING",12,FontStyle.Bold,Color.FromArgb(16,135,104),30,24);
                Text(g,"Website launch",30,FontStyle.Bold,Color.FromArgb(28,38,35),28,56);
                Text(g,"Sprint 12  |  September 2026",14,FontStyle.Regular,Color.FromArgb(100,110,106),30,105);
                using(var pen=new Pen(Color.FromArgb(225,232,229))) g.DrawLine(pen,30,145,690,145);
                Text(g,"DELIVERABLE",11,FontStyle.Bold,Color.FromArgb(105,116,111),30,166);
                Text(g,"OWNER",11,FontStyle.Bold,Color.FromArgb(105,116,111),385,166);
                Text(g,"STATUS",11,FontStyle.Bold,Color.FromArgb(105,116,111),555,166);
                string[] names={"Homepage design","Accessibility review","Release checklist"};
                string[] owners={"Design","Engineering","Product"};
                string[] states={"Complete","In progress","Ready"};
                for(int i=0;i<3;i++) {
                    int row=204+i*53;
                    Text(g,names[i],15,FontStyle.Regular,Color.FromArgb(28,38,35),30,row);
                    Text(g,owners[i],14,FontStyle.Regular,Color.FromArgb(90,101,96),385,row);
                    Text(g,states[i],14,FontStyle.Bold,i==1?Color.FromArgb(40,100,180):Color.FromArgb(16,135,104),555,row);
                    using(var pen=new Pen(Color.FromArgb(235,239,237))) g.DrawLine(pen,30,row+37,690,row+37);
                }
            };
            backdrop.Show(); source.Show(); Pump(300);
            // Only this launch exposes the outline to screenshot APIs.
            var process=Process.Start(executable,"--documentation");
            IntPtr selector=IntPtr.Zero;
            try {
                Pump(2500);
                var output=FindWindow("RegShareWindowClass",null);
                selector=FindWindow("RegionShareSelector",null);
                if(output==IntPtr.Zero || selector==IntPtr.Zero) throw new Exception("RegionShare did not start.");
                SetWindowPos(output,new IntPtr(-1),x+830,y+88,622,465,0x10);
                int dpi=(int)GetDpiForWindow(selector),b=(5*dpi+48)/96,t=(28*dpi+48)/96;
                SetWindowPos(selector,IntPtr.Zero,x+48-b,y+116-t,720+2*b,405+t+b,0x14);
                SetForegroundWindow(output);
                Pump(2000);
                Save(new Rectangle(x,y,1500,610),System.IO.Path.Combine(directory,"overview.png"));
                int mx=x+850,my=y+145;
                PostMessage(output,0x007B,IntPtr.Zero,new IntPtr((my<<16)|(mx&0xffff)));
                Pump(500);
                Save(new Rectangle(x+820,y+78,642,490),System.IO.Path.Combine(directory,"controls.png"));
                PostMessage(output,0x001F,IntPtr.Zero,IntPtr.Zero);
                Console.WriteLine("Saved real application screenshots using a synthetic sample document.");
            } finally {
                if(!process.HasExited) { process.CloseMainWindow(); Pump(500); }
                if(!process.HasExited) process.Kill();
                process.Dispose();
                source.Close(); backdrop.Close();
            }
        }
    }
}
'@
$destination = Join-Path $PSScriptRoot '..\docs\images'
New-Item -ItemType Directory -Force -Path $destination | Out-Null
[RegionShareScreenshots]::Run((Resolve-Path $Executable).ProviderPath, (Resolve-Path $destination).ProviderPath)
