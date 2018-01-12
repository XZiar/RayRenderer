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
using Microsoft.Win32;

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

            this.Loaded += MainWindow_Loaded;
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs rea)
        {
            OnLog(LogLevel.Info, "WPF", "Window Loaded\n");
            Core.Init();

            Core.test.Resize(glMain.ClientSize.Width & 0xffc0, glMain.ClientSize.Height & 0xffc0);
            this.Closed += (o, e) => { Core.test.Dispose(); Core.test = null; };

            glMain.Draw += Core.test.Draw;
            glMain.Resize += (o, e) => { Core.test.Resize(e.Width & 0xffc0, e.Height & 0xffc0); };
        }

        private void btnOpObj_Click(object sender, RoutedEventArgs e)
        {

        }

        private async void btnAddModel_ClickAsync(object sender, RoutedEventArgs e)
        {
            var dlg = new OpenFileDialog()
            {
                Filter = "Wavefront obj files(.obj)|*.obj",
                Title = "导入obj模型",
                AddExtension = true,
                CheckFileExists = true,
                CheckPathExists = true,
                Multiselect = false,
            };
            if (dlg.ShowDialog() == true)
            {
                string fname = dlg.FileName;
                if (await Core.test.AddModelAsync(fname))
                {
                    Core.curObj = ushort.MaxValue;
                    Core.Rotate(-90, 0, 0, Core.OPObject.Object);
                    Core.Move(-1, 0, 0, Core.OPObject.Object);
                    glMain.Invalidate();
                }
            }
        }

        private void btnAddLight_Click(object sender, RoutedEventArgs e)
        {
            switch (Core.test.Lights.Size)
            {
            case 0:
                Core.test.Lights.Add(Basic3D.LightType.Parallel);
                break;
            case 1:
                Core.test.Lights.Add(Basic3D.LightType.Point);
                break;
            case 2:
                Core.test.Lights.Add(Basic3D.LightType.Spot);
                break;
            default:
                Core.test.Lights.Clear();
                break;
            }
            glMain.Invalidate();
            if (Core.test.Lights.Size > 0)
            {
                var lgt = Core.test.Lights[Core.test.Lights.Size - 1];
                Console.WriteLine("get light[{0}], name[{1}] --- [{2}]", Core.test.Lights.Size - 1, lgt.name(), lgt);
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
                var ret = await Core.test.TryAsync();
                Console.WriteLine($"finish calling async, ret is {ret}");
            }
            catch(Exception ex)
            {
                throw new Exception("received exception", ex);
            }
        }

        private void wfh_DpiChanged(object sender, DpiChangedEventArgs e)
        {
            float scaleFactor = (float)(e.NewDpi.PixelsPerDip / e.OldDpi.PixelsPerDip);

            // This method recursively scales all child Controls.
            glMain.Scale(new System.Drawing.SizeF(scaleFactor, scaleFactor)); ;

            // Scale the root control's font
            glMain.Font = new System.Drawing.Font(glMain.Font.FontFamily, glMain.Font.Size * scaleFactor, glMain.Font.Style);
        }

        private void wfh_KeyUp(object sender, System.Windows.Input.KeyEventArgs e)
        {
            Console.WriteLine(e.Key);
            if (e.Key == System.Windows.Input.Key.Return)
            {
                Core.mode = !Core.mode;
                glMain.Invalidate();
            }
        }

        private void OnKeyAction(object sender, KeyBoardEventArgs e)
        {
            Console.WriteLine($"KeyAction {e.key}");
            switch (e.SpecialKey())
            {
                case Key.Up:
                    Core.Move(0, 0.1f, 0, Core.OPObject.Object); break;
                case Key.Down:
                    Core.Move(0, -0.1f, 0, Core.OPObject.Object); break;
                case Key.Left:
                    Core.Move(-0.1f, 0, 0, Core.OPObject.Object); break;
                case Key.Right:
                    Core.Move(0.1f, 0, 0, Core.OPObject.Object); break;
                case Key.PageUp:
                    Core.Move(0, 0, -0.1f, Core.OPObject.Object); break;
                case Key.PageDown:
                    Core.Move(0, 0, 0.1f, Core.OPObject.Object); break;
                default:
                    switch (e.key)
                    {
                        case 'a'://pan to left
                            Core.Rotate(0, 3, 0, Core.OPObject.Camera); break;
                        case 'd'://pan to right
                            Core.Rotate(0, -3, 0, Core.OPObject.Camera); break;
                        case 'w'://pan to up
                            Core.Rotate(3, 0, 0, Core.OPObject.Camera); break;
                        case 's'://pan to down
                            Core.Rotate(-3, 0, 0, Core.OPObject.Camera); break;
                        case 'q'://pan to left
                            Core.Rotate(0, 0, -3, Core.OPObject.Camera); break;
                        case 'e'://pan to left
                            Core.Rotate(0, 0, 3, Core.OPObject.Camera); break;
                        case 'A':
                            Core.Rotate(0, 3, 0, Core.OPObject.Object); break;
                        case 'D':
                            Core.Rotate(0, -3, 0, Core.OPObject.Object); break;
                        case 'W':
                            Core.Rotate(3, 0, 0, Core.OPObject.Object); break;
                        case 'S':
                            Core.Rotate(-3, 0, 0, Core.OPObject.Object); break;
                        case 'Q':
                            Core.Rotate(0, 0, 3, Core.OPObject.Object); break;
                        case 'E':
                            Core.Rotate(0, 0, -3, Core.OPObject.Object); break;
                        case (char)13:
                            if (e.hasShift())
                                Core.isAnimate = !Core.isAnimate;
                            else
                                Core.mode = !Core.mode;
                            break;
                        default:
                            break;
                        case '+':
                            Core.curObj++;
                            break;
                        case '-':
                            Core.curObj--;
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
                    Core.Move((e.dx * 10.0f / Core.test.cam.Width), (e.dy * 10.0f / Core.test.cam.Height), 0, Core.OPObject.Camera);
                    break;
                case MouseEventType.Wheel:
                    Core.Move(0, 0, (float)e.dx, Core.OPObject.Camera);
                    break;
                default:
                    return;
            }
            (sender as OGLView).Invalidate();
        }

        private async void OnDropFileAsync(object sender, System.Windows.Forms.DragEventArgs e)
        {
            string fname = (e.Data.GetData(System.Windows.Forms.DataFormats.FileDrop) as Array).GetValue(0).ToString();
            try
            {
                if (fname.EndsWith(".obj", StringComparison.CurrentCultureIgnoreCase))
                {
                    if (await Core.test.AddModelAsync(fname))
                    {
                        Core.curObj = ushort.MaxValue;
                        Core.Rotate(-90, 0, 0, Core.OPObject.Object);
                        Core.Move(-1, 0, 0, Core.OPObject.Object);
                        glMain.Invalidate();
                    }
                }
                else if (fname.EndsWith(".cl", StringComparison.CurrentCultureIgnoreCase))
                {
                    await Core.test.ReloadCLAsync(fname);
                }
            }
            catch(Exception ex)
            {
                new ExceptionDialog(ex).ShowDialog();
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
