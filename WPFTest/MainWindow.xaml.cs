using common;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using OpenGLView;
using RayRender;

namespace WPFTest
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            par = dbgOutput.Document.Blocks.FirstBlock as Paragraph;
            Logger.OnLog += OnLog;
            
            Main.test.Resize(glMain.ClientSize.Width & 0xffc0, glMain.ClientSize.Height & 0xffc0);
            this.Closed += (o, e) => { Main.test.Dispose(); Main.test = null; };

            glMain.Draw += Main.test.Draw;
            glMain.Resize += (o, e) => { Main.test.Resize(e.Width & 0xffc0, e.Height & 0xffc0); };
        }

        private void btnOpObj_Click(object sender, RoutedEventArgs e)
        {

        }

        private void btnAddModel_Click(object sender, RoutedEventArgs e)
        {

        }

        private void btnAddLight_Click(object sender, RoutedEventArgs e)
        {
            switch (Main.test.Lights.Size)
            {
            case 0:
                Main.test.Lights.Add(Basic3D.LightType.Parallel);
                break;
            case 1:
                Main.test.Lights.Add(Basic3D.LightType.Point);
                break;
            case 2:
                Main.test.Lights.Add(Basic3D.LightType.Spot);
                break;
            default:
                Main.test.Lights.Clear();
                break;
            }
            glMain.Invalidate();
            if (Main.test.Lights.Size > 0)
            {
                var lgt = Main.test.Lights[Main.test.Lights.Size - 1];
                Console.WriteLine("get light[{0}], name[{1}] --- [{2}]", Main.test.Lights.Size - 1, lgt.name(), lgt);
            }
        }

        
        private void OnLog(LogLevel lv, string from, string content)
        {
            dbgOutput.Dispatcher.BeginInvoke(new Action(() => 
            {
                Run r = new Run($"[{from}]{content}");
                switch(lv)
                {
                case LogLevel.Error:
                    r.Foreground = errBrs; break;
                case LogLevel.Warning:
                    r.Foreground = warnBrs; break;
                case LogLevel.Sucess:
                    r.Foreground = sucBrs; break;
                case LogLevel.Info:
                    r.Foreground = infoBrs; break;
                case LogLevel.Verbose:
                    r.Foreground = vbBrs; break;
                case LogLevel.Debug:
                    r.Foreground = dbgBrs; break;
                }
                par.Inlines.Add(r);
                dbgOutput.ScrollToEnd();
            }), System.Windows.Threading.DispatcherPriority.Normal, null);
        }
        private Paragraph par = null;
        private static SolidColorBrush errBrs = new SolidColorBrush(Colors.Red),
            warnBrs = new SolidColorBrush(Colors.Yellow),
            sucBrs = new SolidColorBrush(Colors.Green),
            infoBrs = new SolidColorBrush(Colors.White),
            vbBrs = new SolidColorBrush(Colors.Pink),
            dbgBrs = new SolidColorBrush(Colors.Cyan);

        private async void btnTryAsync_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var ret = await Main.test.TryAsync();
                ret();
                Console.WriteLine("finish calling async");
            }
            catch(Exception ex)
            {
                throw new Exception("received exception", ex);
            }
        }

        private void wfh_KeyUp(object sender, System.Windows.Input.KeyEventArgs e)
        {
            Console.WriteLine(e.Key);
            if (e.Key == System.Windows.Input.Key.Return)
            {
                Main.mode = !Main.mode;
                glMain.Invalidate();
            }
        }

        private void OnKeyAction(object sender, KeyBoardEventArgs e)
        {
            Console.WriteLine($"KeyAction {e.key}");
            switch (e.SpecialKey())
            {
                case Key.Up:
                    Main.Move(0, 0.1f, 0, Main.OPObject.Object); break;
                case Key.Down:
                    Main.Move(0, -0.1f, 0, Main.OPObject.Object); break;
                case Key.Left:
                    Main.Move(-0.1f, 0, 0, Main.OPObject.Object); break;
                case Key.Right:
                    Main.Move(0.1f, 0, 0, Main.OPObject.Object); break;
                case Key.PageUp:
                    Main.Move(0, 0, -0.1f, Main.OPObject.Object); break;
                case Key.PageDown:
                    Main.Move(0, 0, 0.1f, Main.OPObject.Object); break;
                default:
                    switch (e.key)
                    {
                        case 'a'://pan to left
                            Main.Rotate(0, 3, 0, Main.OPObject.Camera); break;
                        case 'd'://pan to right
                            Main.Rotate(0, -3, 0, Main.OPObject.Camera); break;
                        case 'w'://pan to up
                            Main.Rotate(3, 0, 0, Main.OPObject.Camera); break;
                        case 's'://pan to down
                            Main.Rotate(-3, 0, 0, Main.OPObject.Camera); break;
                        case 'q'://pan to left
                            Main.Rotate(0, 0, -3, Main.OPObject.Camera); break;
                        case 'e'://pan to left
                            Main.Rotate(0, 0, 3, Main.OPObject.Camera); break;
                        case 'A':
                            Main.Rotate(0, 3, 0, Main.OPObject.Object); break;
                        case 'D':
                            Main.Rotate(0, -3, 0, Main.OPObject.Object); break;
                        case 'W':
                            Main.Rotate(3, 0, 0, Main.OPObject.Object); break;
                        case 'S':
                            Main.Rotate(-3, 0, 0, Main.OPObject.Object); break;
                        case 'Q':
                            Main.Rotate(0, 0, 3, Main.OPObject.Object); break;
                        case 'E':
                            Main.Rotate(0, 0, -3, Main.OPObject.Object); break;
                        case (char)13:
                            if (e.hasShift())
                                Main.isAnimate = !Main.isAnimate;
                            else
                                Main.mode = !Main.mode;
                            break;
                        default:
                            break;
                        case '+':
                            Main.curObj++;
                            break;
                        case '-':
                            Main.curObj--;
                            break;
                    }
                    break;
            }
            (sender as OGLView).Invalidate();
        }

        private void OnMouse(object sender, MouseEventExArgs e)
        {
            switch (e.type)
            {
                case MouseEventType.Moving:
                    Main.Move((e.dx * 10.0f / Main.test.cam.Width), (e.dy * 10.0f / Main.test.cam.Height), 0, Main.OPObject.Camera);
                    break;
                case MouseEventType.Wheel:
                    Main.Move(0, 0, (float)e.dx, Main.OPObject.Camera);
                    break;
                default:
                    return;
            }
            (sender as OGLView).Invalidate();
        }

        private async void OnDropFileAsync(object sender, System.Windows.Forms.DragEventArgs e)
        {
            string fname = (e.Data.GetData(System.Windows.Forms.DataFormats.FileDrop) as Array).GetValue(0).ToString();
            if (fname.EndsWith(".obj", StringComparison.CurrentCultureIgnoreCase))
            {
                var cb = await Main.test.AddModelAsync(fname);
                if (cb())
                {
                    Main.curObj = ushort.MaxValue;
                    Main.Rotate(-90, 0, 0, Main.OPObject.Object);
                    Main.Move(-1, 0, 0, Main.OPObject.Object);
                    (sender as OGLView).Invalidate();
                }
            }
            else if (fname.EndsWith(".cl", StringComparison.CurrentCultureIgnoreCase))
            {
                Main.test.ReLoadCL(fname);
            }
        }

        private void OnDragEnter(object sender, System.Windows.Forms.DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
                e.Effect = System.Windows.Forms.DragDropEffects.Link;
            else e.Effect = System.Windows.Forms.DragDropEffects.None;
        }
    }
}
