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
using static XZiar.Util.BindingHelper;
using Basic3D;

namespace WPFTest
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        private TestCore Core = null;
        private OPObject OperateTarget = OPObject.Camera;
        private ImageSource imgCamera, imgModel, imgPointLight;
        public MainWindow()
        {
            InitializeComponent();
            imgCamera = this.FindResource("imgCamera") as ImageSource;
            imgModel = this.FindResource("imgModel") as ImageSource;
            imgPointLight = this.FindResource("imgPointLight") as ImageSource;
            //System.Windows.Forms.Integration.WindowsFormsHost.EnableWindowsFormsInterop();
            Logger.OnLog += OnLog;

            this.Loaded += MainWindow_Loaded;

            wfh.IsKeyboardFocusWithinChanged += (o, e) => 
            {
                if ((bool)e.NewValue == true)
                    wfh.TabInto(new System.Windows.Input.TraversalRequest(System.Windows.Input.FocusNavigationDirection.First));
                //Console.WriteLine($"kbFocued:{e.NewValue}, now at {System.Windows.Input.Keyboard.FocusedElement}");
            };
            System.Windows.Input.Keyboard.Focus(wfh);
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs rea)
        {
            OnLog(LogLevel.Info, "WPF", "Window Loaded\n");
            Core = new TestCore();

            Core.Test.Resize(glMain.ClientSize.Width & 0xffc0, glMain.ClientSize.Height & 0xffc0);
            this.Closed += (o, e) => { Core.Dispose(); Core = null; };

            glMain.Draw += Core.Test.Draw;
            glMain.Resize += (o, e) => { Core.Test.Resize(e.Width & 0xffc0, e.Height & 0xffc0); };

            txtCurObj.SetBinding(TextBlock.TextProperty, new Binding
            {
                Source = Core,
                Path = new PropertyPath("CurObj"),
                Mode = BindingMode.OneWay,
                Converter = new OneWayValueConvertor(o => 
                {
                    if (o == null) return "No object selected";
                    var obj = o as Drawable;
                    return $"#{Core.CurObjIdx}[{obj.Type}] {obj.Name}";
                })
            });
            txtCurLight.SetBinding(TextBlock.TextProperty, new Binding
            {
                Source = Core,
                Path = new PropertyPath("CurLgt"),
                Mode = BindingMode.OneWay,
                Converter = new OneWayValueConvertor(o =>
                {
                    if (o == null) return "No light selected";
                    var light = o as Light;
                    return $"#{Core.CurLgtIdx}[{light.Type}] {light.Name}";
                })
            });

            glMain.Invalidate();
        }

        private static readonly Dictionary<LogLevel, SolidColorBrush> brashMap = new Dictionary<LogLevel, SolidColorBrush>
        {
            { LogLevel.Error,   new SolidColorBrush(Colors.Red)     },
            { LogLevel.Warning, new SolidColorBrush(Colors.Yellow)  },
            { LogLevel.Success, new SolidColorBrush(Colors.Green)   },
            { LogLevel.Info,    new SolidColorBrush(Colors.White)   },
            { LogLevel.Verbose, new SolidColorBrush(Colors.Pink)    },
            { LogLevel.Debug,   new SolidColorBrush(Colors.Cyan)    }
        };
        private void OnLog(LogLevel level, string from, string content)
        {
            dbgOutput.Dispatcher.BeginInvoke(new Action(() => 
            {
                Run r = new Run($"[{from}]{content}")
                {
                    Foreground = brashMap[level]
                };
                para.Inlines.Add(r);
                dbgOutput.ScrollToEnd();
            }), System.Windows.Threading.DispatcherPriority.Normal, null);
        }

        private void btnDragMode_Click(object sender, RoutedEventArgs e)
        {
            var btnImg = btnDragMode.Content as Image;
            switch (OperateTarget)
            {
            case OPObject.Camera:
                OperateTarget = OPObject.Drawable;
                btnImg.Source = imgModel;
                break;
            case OPObject.Drawable:
                OperateTarget = OPObject.Light;
                btnImg.Source = imgPointLight;
                break;
            case OPObject.Light:
                OperateTarget = OPObject.Camera;
                btnImg.Source = imgCamera;
                break;
            }
        }
        private void btncmDragMode_Click(object sender, RoutedEventArgs e)
        {
            var btnImg = btnDragMode.Content as Image;
            MenuItem mi = e.OriginalSource as MenuItem;
            switch (mi.Tag as string)
            {
            case "camera":
                OperateTarget = OPObject.Camera;
                btnImg.Source = imgCamera;
                break;
            case "object":
                OperateTarget = OPObject.Drawable;
                btnImg.Source = imgModel;
                break;
            case "light":
                OperateTarget = OPObject.Light;
                btnImg.Source = imgPointLight;
                break;
            }
        }

        private void OpenModelInputDialog()
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
                AddModelAsync(dlg.FileName);
            }
        }

        private async void AddModelAsync(string fileName)
        {
            if (await Core.Test.AddModelAsync(fileName))
            {
                Core.CurObjIdx = ushort.MaxValue;
                ShowCurrentObject();
                Core.Rotate(-90, 0, 0, OPObject.Drawable);
                Core.Move(-1, 0, 0, OPObject.Drawable);
                glMain.Invalidate();
            }
        }

        private void btnAddModel_Click(object sender, RoutedEventArgs e)
        {
            OpenModelInputDialog();
        }

        private void btncmAddModel_Click(object sender, RoutedEventArgs e)
        {
            MenuItem mi = e.OriginalSource as MenuItem;
            switch (mi.Tag as string)
            {
            case "cube":
                break;
            case "sphere":
                break;
            case "plane":
                break;
            case "model":
                OpenModelInputDialog();
                break;
            default:
                return;
            }
            glMain.Invalidate();
        }

        private void btnAddLight_Click(object sender, RoutedEventArgs e)
        {
            btncmAddLight.PlacementTarget = btnAddLight;
            btncmAddLight.IsOpen = true;
        }

        private void btncmAddLight_Click(object sender, RoutedEventArgs e)
        {
            MenuItem mi = e.OriginalSource as MenuItem;
            switch(mi.Tag as string)
            {
            case "parallel":
                Core.Test.Lights.Add(Basic3D.LightType.Parallel);
                break;
            case "point":
                Core.Test.Lights.Add(Basic3D.LightType.Point);
                break;
            case "spot":
                Core.Test.Lights.Add(Basic3D.LightType.Spot);
                break;
            }
            glMain.Invalidate();
            Core.CurLgtIdx = ushort.MaxValue;
        }

        private async void btnTryAsync_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var ret = await Core.Test.TryAsync();
                Console.WriteLine($"finish calling async, ret is {ret}");
            }
            catch (Exception ex)
            {
                throw new Exception("received exception", ex);
            }
        }

        private void ShowCurrentObject()
        {
            var obj = Core.CurObj;
            Console.WriteLine("get drawable[{0}], name[{1}] --- [{2}]", Core.CurObjIdx, obj.Name, obj.Type);
        }

        private void OnKeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            Console.WriteLine($"KeyDown value[{e.KeyValue}] code[{e.KeyCode}] data[{e.KeyData}]");
            switch (e.KeyCode)
            {
                case System.Windows.Forms.Keys.Up:
                    Core.Move(0, 0.1f, 0, OPObject.Drawable); break;
                case System.Windows.Forms.Keys.Down:
                    Core.Move(0, -0.1f, 0, OPObject.Drawable); break;
                case System.Windows.Forms.Keys.Left:
                    Core.Move(-0.1f, 0, 0, OPObject.Drawable); break;
                case System.Windows.Forms.Keys.Right:
                    Core.Move(0.1f, 0, 0, OPObject.Drawable); break;
                case System.Windows.Forms.Keys.PageUp:
                    Core.Move(0, 0, -0.1f, OPObject.Drawable); break;
                case System.Windows.Forms.Keys.PageDown:
                    Core.Move(0, 0, 0.1f, OPObject.Drawable); break;
                case System.Windows.Forms.Keys.Add:
                    Core.CurObjIdx++; ShowCurrentObject(); break;
                case System.Windows.Forms.Keys.Subtract:
                    Core.CurObjIdx--; ShowCurrentObject(); break;
                default:
                    if (e.Shift)
                    {
                        switch (e.KeyValue)
                        {
                            case 'A'://pan to left
                                Core.Rotate(0, 3, 0, OPObject.Drawable); break;
                            case 'D'://pan to right
                                Core.Rotate(0, -3, 0, OPObject.Drawable); break;
                            case 'W'://pan to up
                                Core.Rotate(3, 0, 0, OPObject.Drawable); break;
                            case 'S'://pan to down
                                Core.Rotate(-3, 0, 0, OPObject.Drawable); break;
                            case 'Q'://pan to left
                                Core.Rotate(0, 0, -3, OPObject.Drawable); break;
                            case 'E'://pan to left
                                Core.Rotate(0, 0, 3, OPObject.Drawable); break;
                            case '\r':
                                Core.IsAnimate = !Core.IsAnimate; break;
                        }
                    }
                    else
                    {
                        switch (e.KeyValue)
                        {
                            case 'A'://pan to left
                                Core.Rotate(0, 3, 0, OPObject.Camera); break;
                            case 'D'://pan to right
                                Core.Rotate(0, -3, 0, OPObject.Camera); break;
                            case 'W'://pan to up
                                Core.Rotate(3, 0, 0, OPObject.Camera); break;
                            case 'S'://pan to down
                                Core.Rotate(-3, 0, 0, OPObject.Camera); break;
                            case 'Q'://pan to left
                                Core.Rotate(0, 0, -3, OPObject.Camera); break;
                            case 'E'://pan to left
                                Core.Rotate(0, 0, 3, OPObject.Camera); break;
                            case '\r':
                                Core.Mode = !Core.Mode; break;
                        }
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
                    Core.Move((e.dx * 10.0f / Core.Test.Camera.Width), (e.dy * 10.0f / Core.Test.Camera.Height), 0, OPObject.Camera);
                    break;
                case MouseEventType.Wheel:
                    Core.Move(0, 0, (float)e.dx, OPObject.Camera);
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
                    AddModelAsync(fname);
                }
                else if (fname.EndsWith(".cl", StringComparison.CurrentCultureIgnoreCase))
                {
                    if (await Core.Test.ReloadCLAsync(fname))
                        glMain.Invalidate();
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
