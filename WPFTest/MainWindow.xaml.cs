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
using XZiar.WPFControl;
using XZiar.Util;
using System.Windows.Threading;
using System.Threading;
using OpenGLUtil;

namespace WPFTest
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        private static BrushConverter Brush_Conv = new BrushConverter();
        private TestCore Core = null;
        private MemoryMonitor MemMonitor = null;
        private OPObject OperateTarget = OPObject.Camera;
        private ImageSource imgCamera, imgCube, imgPointLight;
        private readonly SolidColorBrush brshBlue = Brush_Conv.ConvertFromString("#FF007ACC") as SolidColorBrush,
            brshOrange = Brush_Conv.ConvertFromString("#FFDC5E07") as SolidColorBrush;
        private Timer AutoRefresher;
        private float MouseSensative => (float)slMouseSen.Value;
        private float ScrollSensative => (float)slScrollSen.Value;
        private ushort waitingCount = 0;
        public ushort WaitingCount { get { return waitingCount; } set { waitingCount = value; ChangeStatusBar(value); } }

        private void ChangeStatusBar(ushort value)
        {
            barStatus.Background = value > 0 ? brshOrange : brshBlue;
        }

        public MainWindow()
        {
            InitializeComponent();
            XZiar.Util.BaseViewModel.Init();
            common.BaseViewModel.Init();
            MemMonitor = new MemoryMonitor();
            imgCamera = (ImageSource)this.FindResource("imgCamera");
            imgCube = (ImageSource)this.FindResource("imgCube");
            imgPointLight = (ImageSource)this.FindResource("imgPointLight");
            brshBlue = (SolidColorBrush)this.FindResource("brshBlue");
            brshOrange = (SolidColorBrush)this.FindResource("brshOrange");
            common.Logger.OnLog += OnLog;

            wfh.IsKeyboardFocusWithinChanged += (o, e) => 
            {
                if ((bool)e.NewValue == true)
                    wfh.TabInto(new System.Windows.Input.TraversalRequest(System.Windows.Input.FocusNavigationDirection.First));
                //Console.WriteLine($"kbFocued:{e.NewValue}, now at {System.Windows.Input.Keyboard.FocusedElement}");
            };
            System.Windows.Input.Keyboard.Focus(wfh);

            InitializeCore();
        }

        private void InitializeCore()
        {
            OnLog(common.LogLevel.Info, "WPF", "Window Loaded\n");
            Core = new TestCore();

            Core.Test.Resize(glMain.ClientSize.Width & 0xffc0, glMain.ClientSize.Height & 0xffc0);
            this.Closed += (o, e) => { Core.Dispose(); Core = null; };

            glMain.Draw += Core.Test.Draw;
            glMain.Resize += (o, e) => { Core.Test.Resize(e.Width & 0xffc0, e.Height & 0xffc0); };

            txtMemInfo.SetBinding(TextBlock.TextProperty, new Binding
            {
                Source = MemMonitor,
                Path = new PropertyPath("Current"),
                Mode = BindingMode.OneWay,
                Converter = new OneWayValueConvertor(o => 
                {
                    var monitor = (MemoryMonitor)o;
                    string ParseSize(ulong size)
                    {
                        if (size > 1024 * 1024 * 1024)
                            return string.Format("{0:F2}G", size * 1.0 / (1024 * 1024 * 1024));
                        else if (size > 1024 * 1024)
                            return string.Format("{0:F2}M", size * 1.0 / (1024 * 1024));
                        else if (size > 1024)
                            return string.Format("{0:F2}K", size * 1.0 / (1024));
                        else
                            return string.Format("{0:D}B", size);
                    }
                    return ParseSize(monitor.ManagedSize) + " / " + ParseSize(monitor.PrivateSize) + " / " + ParseSize(monitor.WorkingSet);
                })
            });
            MemMonitor.UpdateInterval(1000, true);

            txtCurCamera.SetBinding(TextBlock.TextProperty, new Binding
            {
                Source = Core.Camera,
                Path = new PropertyPath("Position"),
                Mode = BindingMode.OneWay
            });
            txtCurObj.SetBinding(TextBlock.TextProperty, new Binding
            {
                Source = Core.Drawables,
                Path = new PropertyPath("Current"),
                Mode = BindingMode.OneWay,
                Converter = new OneWayValueConvertor(o => 
                {
                    if (o == null) return "No object selected";
                    var obj = o as Drawable;
                    return $"#{Core.Drawables.CurObjIdx}[{obj.Type}] {obj.Name}";
                })
            });
            txtCurLight.SetBinding(TextBlock.TextProperty, new Binding
            {
                Source = Core.Lights,
                Path = new PropertyPath("Current"),
                Mode = BindingMode.OneWay,
                Converter = new OneWayValueConvertor(o =>
                {
                    if (o == null) return "No light selected";
                    var light = o as Light;
                    return $"#{Core.Lights.CurLgtIdx}[{light.Type}] {light.Name}";
                })
            });
            txtCurShd.SetBinding(TextBlock.TextProperty, new Binding
            {
                Source = Core.Shaders,
                Path = new PropertyPath("Current"),
                Mode = BindingMode.OneWay,
                Converter = new OneWayValueConvertor(o => ((GLProgram)o)?.Name)
            });

            cboxFCull.ItemsSource = new[] { FaceCullingType.OFF, FaceCullingType.CullCW, FaceCullingType.CullCCW, FaceCullingType.CullAll };
            cboxFCull.SelectedItem = FaceCullingType.OFF;
            cboxFCull.SelectionChanged += (o, e) =>
            {
                var item = e.AddedItems.Cast<FaceCullingType?>().FirstOrDefault();
                if (item.HasValue)
                {
                    Core.Test.SetFaceCulling(item.Value);
                    glMain.Invalidate();
                }
            };
            cboxLight.SetBinding(ComboBox.ItemsSourceProperty, new Binding
            {
                Source = Core.Lights,
                Mode = BindingMode.OneWay
            });
            cboxLight.SetBinding(ComboBox.SelectedItemProperty, new Binding
            {
                Source = Core.Lights,
                Path = new PropertyPath("Current"),
                Mode = BindingMode.TwoWay
            });
            cboxObj.SetBinding(ComboBox.ItemsSourceProperty, new Binding
            {
                Source = Core.Drawables,
                Mode = BindingMode.OneWay
            });
            cboxObj.SetBinding(ComboBox.SelectedItemProperty, new Binding
            {
                Source = Core.Drawables,
                Path = new PropertyPath("Current"),
                Mode = BindingMode.TwoWay
            });
            cboxShader.SetBinding(ComboBox.ItemsSourceProperty, new Binding
            {
                Source = Core.Shaders,
                Mode = BindingMode.OneWay
            });

            AutoRefresher = new Timer(o =>
            {
                if (Core.IsAnimate)
                {
                    Core.Rotate(0, 3, 0, OPObject.Drawable);
                    this.Dispatcher.InvokeAsync(() => glMain.Invalidate(), System.Windows.Threading.DispatcherPriority.Normal);
                }
            }, null, 0, 20);
            this.Closing += (o, e) => AutoRefresher.Change(Timeout.Infinite, 20);
            glMain.Invalidate();
            WaitingCount = 0;
        }

        private static readonly Dictionary<common.LogLevel, SolidColorBrush> brashMap = new Dictionary<common.LogLevel, SolidColorBrush>
        {
            { common.LogLevel.Error,   new SolidColorBrush(Colors.Red)     },
            { common.LogLevel.Warning, new SolidColorBrush(Colors.Yellow)  },
            { common.LogLevel.Success, new SolidColorBrush(Colors.LawnGreen)   },
            { common.LogLevel.Info,    new SolidColorBrush(Colors.White)   },
            { common.LogLevel.Verbose, new SolidColorBrush(Colors.Pink)    },
            { common.LogLevel.Debug,   new SolidColorBrush(Colors.Cyan)    }
        };
        private void OnLog(common.LogLevel level, string from, string content)
        {
            dbgOutput.Dispatcher.InvokeAsync(() => 
            {
                Run r = new Run($"[{from}]{content}")
                {
                    Foreground = brashMap[level]
                };
                para.Inlines.Add(r);
                if (dbgScroll.ScrollableHeight - dbgScroll.VerticalOffset < 120)
                    dbgScroll.ScrollToEnd();
            }, System.Windows.Threading.DispatcherPriority.Normal);
        }

        private void btnDragMode_Click(object sender, RoutedEventArgs e)
        {
            var btnImg = (ImageBrush)btnDragMode.Content;
            switch (OperateTarget)
            {
            case OPObject.Camera:
                OperateTarget = OPObject.Drawable;
                btnImg.ImageSource = imgCube;
                break;
            case OPObject.Drawable:
                OperateTarget = OPObject.Light;
                btnImg.ImageSource = imgPointLight;
                break;
            case OPObject.Light:
                OperateTarget = OPObject.Camera;
                btnImg.ImageSource = imgCamera;
                break;
            }
            e.Handled = true;
        }
        private void btncmDragMode_Click(object sender, RoutedEventArgs e)
        {
            var btnImg = (ImageBrush)btnDragMode.Content;
            var mi = (MenuItem)e.OriginalSource;
            switch (mi.Tag as string)
            {
            case "camera":
                OperateTarget = OPObject.Camera;
                btnImg.ImageSource = imgCamera;
                break;
            case "object":
                OperateTarget = OPObject.Drawable;
                btnImg.ImageSource = imgCube;
                break;
            case "light":
                OperateTarget = OPObject.Light;
                btnImg.ImageSource = imgPointLight;
                break;
            }
            e.Handled = true;
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
            try
            {
                WaitingCount++;
                if (await Core.Drawables.AddModelAsync(fileName))
                {
                    Core.Rotate(90, 0, 0, OPObject.Drawable);
                    Core.Move(-1, 0, 0, OPObject.Drawable);
                    glMain.Invalidate();
                }
            }
            catch (Exception ex)
            {
                new TextDialog(ex).ShowDialog();
            }
            finally
            {
                WaitingCount--;
            }
        }

        private void btnAddModel_Click(object sender, RoutedEventArgs e)
        {
            OpenModelInputDialog();
            e.Handled = true;
        }

        private void btncmAddModel_Click(object sender, RoutedEventArgs e)
        {
            var mi = (MenuItem)e.OriginalSource;
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
            e.Handled = true;
        }

        private void btnAddLight_Click(object sender, RoutedEventArgs e)
        {
            btncmAddLight.PlacementTarget = btnAddLight;
            btncmAddLight.IsOpen = true;
            e.Handled = true;
        }

        private void btncmAddLight_Click(object sender, RoutedEventArgs e)
        {
            var mi = (MenuItem)e.OriginalSource;
            switch (mi.Tag as string)
            {
            case "parallel":
                Core.Lights.Add(Basic3D.LightType.Parallel);
                break;
            case "point":
                Core.Lights.Add(Basic3D.LightType.Point);
                break;
            case "spot":
                Core.Lights.Add(Basic3D.LightType.Spot);
                break;
            }
            glMain.Invalidate();
            e.Handled = true;
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
                    Core.Drawables.CurObjIdx++; break;
                case System.Windows.Forms.Keys.Subtract:
                    Core.Drawables.CurObjIdx--; break;
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
            glMain.Invalidate();
        }

        private void OnMouse(object sender, MouseEventExArgs e)
        {
            switch (e.Type)
            {
                case MouseEventType.Moving:
                    if (e.Button.HasFlag(MouseButton.Left))
                        Core.Move((e.dx * 10.0f / Core.Test.Camera.Width), (e.dy * 10.0f / Core.Test.Camera.Height), 0, OperateTarget);
                    else if (e.Button.HasFlag(MouseButton.Right))
                        Core.Rotate((e.dy * MouseSensative / Core.Test.Camera.Height), (e.dx * -MouseSensative / Core.Test.Camera.Width), 0, OperateTarget); //need to reverse dx
                    break;
                case MouseEventType.Wheel:
                    Core.Move(0, 0, (float)e.dx * ScrollSensative, OperateTarget);
                    break;
                default:
                    return;
            }
            glMain.Invalidate();
        }

        private void GLMainInvalidator(object sender, RoutedEventArgs e)
        {
            glMain.Invalidate();
            e.Handled = true;
        }

        private void btnOpenShaderSrc_Click(object sender, RoutedEventArgs e)
        {
            var shader = (ShaderObject)((Button)sender).DataContext;
            new TextDialog(shader.Source, $"{((GLProgram)stkShader.DataContext).Name} --- {shader.Type}").Show();
            e.Handled = true;
        }

        private void btnUseShader_Click(object sender, RoutedEventArgs e)
        {
            Core.Shaders.UseProgram(cboxShader.SelectedItem as GLProgram);
            glMain.Invalidate();
        }

        private async void OnDropFileAsync(object sender, System.Windows.Forms.DragEventArgs e)
        {
            string fname = (e.Data.GetData(System.Windows.Forms.DataFormats.FileDrop) as Array).GetValue(0).ToString();
            var extName = System.IO.Path.GetExtension(fname).ToLower();
            switch(extName)
            {
            case ".obj":
                AddModelAsync(fname);
                break;
            case ".glsl":
                try
                {
                    WaitingCount++;
                    if (await Core.Shaders.AddShaderAsync(fname))
                        glMain.Invalidate();
                }
                catch (Exception ex)
                {
                    new TextDialog(ex).ShowDialog();
                }
                finally
                {
                    WaitingCount--;
                }
                break;
            case ".cl":
                try
                {
                    WaitingCount++;
                    if (await Core.Test.ReloadCLAsync(fname))
                        glMain.Invalidate();
                }
                catch (Exception ex)
                {
                    new TextDialog(ex).ShowDialog();
                }
                finally
                {
                    WaitingCount--;
                }
                break;
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
