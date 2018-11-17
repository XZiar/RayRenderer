using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Threading;
using Dizz;
using Microsoft.Win32;
using XZiar.Util;

namespace WPFTest
{
    [MarkupExtensionReturnType(typeof(BindingExpression))]
    internal class ThumbnailBinding : MarkupExtension
    {
        private static readonly DependencyProperty TargetProperty = DependencyProperty.RegisterAttached(
            "Target",
            typeof(DependencyProperty),
            typeof(ThumbnailBinding),
            new PropertyMetadata(null));
        private static readonly DependencyProperty TexHolderProperty = DependencyProperty.RegisterAttached(
            "TexHolder",
            typeof(TexHolder),
            typeof(ThumbnailBinding),
            new PropertyMetadata(null, TexHolderChanged));
        internal static readonly WeakReference<ThumbnailMan> Manager = new WeakReference<ThumbnailMan>(null);

        public PropertyPath Path { get; set; }

        static void TexHolderChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is FrameworkElement target) || !(e.NewValue is TexHolder newVal))
                return;
            if (!(target.GetValue(TargetProperty) is DependencyProperty property) || property == null)
                return;
            if (newVal != null && Manager.TryGetTarget(out ThumbnailMan thumbMan))
            {
                var task = thumbMan.GetThumbnailAsync(newVal as TexHolder);
                task.GetAwaiter().OnCompleted(async () =>
                    {
                        if (target.GetValue(TexHolderProperty) == newVal)
                            target.SetValue(property, await task);
                    });
            }
            else
            {
                target.SetValue(property, property.GetMetadata(target).DefaultValue);
            }
        }

        public ThumbnailBinding()
        { }

        public ThumbnailBinding(string path)
        {
            Path = new PropertyPath(path);
        }

        public override object ProvideValue(IServiceProvider serviceProvider)
        {
            var provideValueTargetService = (IProvideValueTarget)serviceProvider.GetService(typeof(IProvideValueTarget));
            if (provideValueTargetService == null)
                return null;
            if (provideValueTargetService.TargetObject != null &&
                provideValueTargetService.TargetObject.GetType().FullName == "System.Windows.SharedDp")
                return this;

            if (!(provideValueTargetService.TargetObject is FrameworkElement targetObject) ||
                !(provideValueTargetService.TargetProperty is DependencyProperty targetProperty))
                return null;

            targetObject.SetValue(TargetProperty, targetProperty);

            var texHolderBinding = new Binding
            {
                Path = Path,
                Mode = BindingMode.OneWay,
            };
            //if (Source != null)
            //    binding.Source = Source;
            targetObject.SetBinding(TexHolderProperty, texHolderBinding);
            return null;
        }
    }

    /// <summary>
    /// TheWindow.xaml 的交互逻辑
    /// </summary>
    public partial class TheWindow : Window
    {
        const float MULER = (float)(Math.PI / 180);
        private static BrushConverter Brush_Conv = new BrushConverter();

        private RenderCore Core = null;
        private LogManager Logger;
        private MemoryMonitor MemMonitor = null;
        private IMovable[] OperateTargets = new IMovable[3];
        private int CurTarget = 0;
        private bool ShouldRefresh = false;
        private ImageSource imgCamera, imgCube, imgPointLight;
        private readonly SolidColorBrush brshBlue = Brush_Conv.ConvertFromString("#FF007ACC") as SolidColorBrush,
            brshOrange = Brush_Conv.ConvertFromString("#FFDC5E07") as SolidColorBrush;
        private Timer AutoRefresher;
        private ushort waitingCount = 0;
        public ushort WaitingCount { get { return waitingCount; } set { waitingCount = value; ChangeStatusBar(value); } }
        private float MouseSensative => (float)60;
        private float ScrollSensative => (float)1;

        private void ChangeStatusBar(ushort value)
        {
            barStatus.Background = value > 0 ? brshOrange : brshBlue;
        }

        public TheWindow()
        {
            InitializeComponent();
            XZiar.Util.BaseViewModel.Init();
            Common.ViewModelSyncRoot.Init();
            MemMonitor = new MemoryMonitor();
            imgCamera = (ImageSource)this.FindResource("imgCamera");
            imgCube = (ImageSource)this.FindResource("imgCube");
            imgPointLight = (ImageSource)this.FindResource("imgPointLight");
            brshBlue = (SolidColorBrush)this.FindResource("brshBlue");
            brshOrange = (SolidColorBrush)this.FindResource("brshOrange");
            Logger = new LogManager(cboxDbgLv, cboxDbgSrc, para, dbgScroll);

            wfh.IsKeyboardFocusWithinChanged += (o, e) =>
            {
                if ((bool)e.NewValue == true)
                    wfh.TabInto(new System.Windows.Input.TraversalRequest(System.Windows.Input.FocusNavigationDirection.First));
                //Console.WriteLine($"kbFocued:{e.NewValue}, now at {System.Windows.Input.Keyboard.FocusedElement}");
            };
            System.Windows.Input.Keyboard.Focus(wfh);
            txtMemInfo.SetBinding(TextBlock.TextProperty, MemMonitor.GetTextBinding());
            MemMonitor.UpdateInterval(1000, true);

            InitializeCore();
        }

        private void InvalidateOnPropChanged(string name)
        {
            if (name != "Name")
                glMain.Invalidate();
        }

        private void InitializeCore()
        {
            Core = new RenderCore();
            ThumbnailBinding.Manager.SetTarget(Core.ThumbMan);
            OperateTargets[0] = Core.TheScene.MainCamera;
            OperateTargets[1] = Core.TheScene.Drawables.LastOrDefault();
            OperateTargets[2] = Core.TheScene.Lights.LastOrDefault();

            Core.Resize((uint)glMain.ClientSize.Width, (uint)glMain.ClientSize.Height);
            this.Closed += (o, e) => { Core.Dispose(); Core = null; };

            glMain.Draw += Core.Draw;
            glMain.Resize += (o, e) => { Core.Resize((uint)e.Width, (uint)e.Height); };

            AutoRefresher = new Timer(o =>
            {
                if (!ShouldRefresh)
                    return;
                OperateTargets[1]?.Rotate(0, 3 * MULER, 0);
                this.Dispatcher.InvokeAsync(() => glMain.Invalidate(), System.Windows.Threading.DispatcherPriority.Normal);
            }, null, 0, 15);
            this.Closing += (o, e) => AutoRefresher.Change(Timeout.Infinite, 20);
            glMain.Invalidate();
            WaitingCount = 0;

            var fpsTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(0.5) };
            fpsTimer.Tick += (o, e) => { var timeUs = glMain?.AvgDrawTime ?? 0; txtFPS.Text = timeUs > 0 ? $"{1000000 / timeUs} FPS@{timeUs / 1000}ms" : ""; };
            fpsTimer.Start();

            Core.TheScene.Drawables.ObjectPropertyChanged += (s, t, e) => InvalidateOnPropChanged(e.PropertyName);
            cboxDrawable.ItemsSource = Core.TheScene.Drawables;
            Core.TheScene.Lights.ObjectPropertyChanged += (s, t, e) => InvalidateOnPropChanged(e.PropertyName);
            cboxLight.ItemsSource = Core.TheScene.Lights;
            Core.Passes.ObjectPropertyChanged += (s, t, e) => InvalidateOnPropChanged(e.PropertyName);
            cboxShader.ItemsSource = Core.Passes;
            Core.Controls.ForEach(x => x.PropertyChanged += (o, e) => InvalidateOnPropChanged(e.PropertyName));
            cboxControl.ItemsSource = Core.Controls;
            Core.TheScene.MainCamera.PropertyChanged += (o, e) => InvalidateOnPropChanged(e.PropertyName);
        }


        private void btnDragMode_Click(object sender, RoutedEventArgs e)
        {
            var btnImg = (ImageBrush)btnDragMode.Content;
            switch (CurTarget)
            {
            case 0:
                CurTarget = 1;
                btnImg.ImageSource = imgCube;
                break;
            case 1:
                CurTarget = 2;
                btnImg.ImageSource = imgPointLight;
                break;
            case 2:
                CurTarget = 0;
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
                CurTarget = 0;
                btnImg.ImageSource = imgCamera;
                break;
            case "object":
                CurTarget = 1;
                btnImg.ImageSource = imgCube;
                break;
            case "light":
                CurTarget = 2;
                btnImg.ImageSource = imgPointLight;
                break;
            }
            e.Handled = true;
        }

        private void OpenModelInputDialog()
        {
            var dlg = new OpenFileDialog()
            {
                Filter = "Wavefront obj files (*.obj)|*.obj",
                Title = "导入obj模型",
                AddExtension = true,
                CheckFileExists = true,
                CheckPathExists = true,
                Multiselect = false,
                ValidateNames = true,
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
                var model = await Core.LoadModelAsync(fileName);
                model.Rotate(-90 * MULER, 0, 0);
                model.Move(-1, 0, 0);
                Core.TheScene.Drawables.Add(model);
                OperateTargets[1] = Core.TheScene.Drawables.LastOrDefault();
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
                Core.TheScene.Lights.Add(Light.NewLight(LightType.Parallel));
                break;
            case "point":
                Core.TheScene.Lights.Add(Light.NewLight(LightType.Point));
                break;
            case "spot":
                Core.TheScene.Lights.Add(Light.NewLight(LightType.Spot));
                break;
            }
            OperateTargets[2] = Core.TheScene.Lights.LastOrDefault();
            glMain.Invalidate();
            e.Handled = true;
        }


        private void OnKeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            Console.WriteLine($"KeyDown value[{e.KeyValue}] code[{e.KeyCode}] data[{e.KeyData}]");
            switch (e.KeyCode)
            {
            case System.Windows.Forms.Keys.Up:
                OperateTargets[CurTarget]?.Move(0, 0.1f, 0); break;
            case System.Windows.Forms.Keys.Down:
                OperateTargets[CurTarget]?.Move(0, -0.1f, 0); break;
            case System.Windows.Forms.Keys.Left:
                OperateTargets[CurTarget]?.Move(-0.1f, 0, 0); break;
            case System.Windows.Forms.Keys.Right:
                OperateTargets[CurTarget]?.Move(0.1f, 0, 0); break;
            case System.Windows.Forms.Keys.PageUp:
                OperateTargets[CurTarget]?.Move(0, 0, -0.1f); break;
            case System.Windows.Forms.Keys.PageDown:
                OperateTargets[CurTarget]?.Move(0, 0, 0.1f); break;
            case System.Windows.Forms.Keys.Add:
                {
                    var idx = Core.TheScene.Drawables.IndexOf(OperateTargets[1] as Drawable) + 1;
                    OperateTargets[1] = Core.TheScene.Drawables.ElementAtOrDefault(idx == Core.TheScene.Drawables.Count ? 0 : idx);
                } break;
            case System.Windows.Forms.Keys.Subtract:
                {
                    var idx = Core.TheScene.Drawables.IndexOf(OperateTargets[1] as Drawable) - 1;
                    OperateTargets[1] = Core.TheScene.Drawables.ElementAtOrDefault(idx < 0 ? Core.TheScene.Drawables.Count - 1 : idx);
                }
                break;
            default:
                if (e.Shift)
                {
                    switch (e.KeyValue)
                    {
                    case 'A'://pan to left
                        OperateTargets[CurTarget]?.Rotate(0, 3 * MULER, 0); break;
                    case 'D'://pan to right
                        OperateTargets[CurTarget]?.Rotate(0, -3 * MULER, 0); break;
                    case 'W'://pan to up
                        OperateTargets[CurTarget]?.Rotate(3 * MULER, 0, 0); break;
                    case 'S'://pan to down
                        OperateTargets[CurTarget]?.Rotate(-3 * MULER, 0, 0); break;
                    case 'Q'://pan to left
                        OperateTargets[CurTarget]?.Rotate(0, 0, -3 * MULER); break;
                    case 'E'://pan to left
                        OperateTargets[CurTarget]?.Rotate(0, 0, 3 * MULER); break;
                    case '\r':
                        ShouldRefresh = !ShouldRefresh; break;
                    }
                }
                else
                {
                    switch (e.KeyValue)
                    {
                    case 'A'://pan to left
                        Core.TheScene.MainCamera.Rotate(0, 3 * MULER, 0); break;
                    case 'D'://pan to right
                        Core.TheScene.MainCamera.Rotate(0, -3 * MULER, 0); break;
                    case 'W'://pan to up
                        Core.TheScene.MainCamera.Rotate(3 * MULER, 0, 0); break;
                    case 'S'://pan to down
                        Core.TheScene.MainCamera.Rotate(-3 * MULER, 0, 0); break;
                    case 'Q'://pan to left
                        Core.TheScene.MainCamera.Rotate(0, 0, -3 * MULER); break;
                    case 'E'://pan to left
                        Core.TheScene.MainCamera.Rotate(0, 0, 3 * MULER); break;
                    }
                }
                break;
            }
            //glMain.Invalidate();
        }

        private void OnMouse(object sender, OpenGLView.MouseEventExArgs e)
        {
            switch (e.Type)
            {
            case OpenGLView.MouseEventType.Moving:
                if (e.Button.HasFlag(OpenGLView.MouseButton.Left))
                    OperateTargets[CurTarget]?.Move((e.dx * 10.0f / Core.Width), (e.dy * 10.0f / Core.Height), 0);
                else if (e.Button.HasFlag(OpenGLView.MouseButton.Right))
                    OperateTargets[CurTarget]?.Rotate((e.dy * MULER * MouseSensative / Core.Height), (e.dx * MULER * -MouseSensative / Core.Width), 0); //need to reverse dx
                break;
            case OpenGLView.MouseEventType.Wheel:
                OperateTargets[CurTarget]?.Move(0, 0, (float)e.dx * ScrollSensative);
                break;
            default:
                return;
            }
            //glMain.Invalidate();
        }


        private void btnScreenshot_Click(object sender, RoutedEventArgs e)
        {
            WaitingCount++;
            var saver = Core.Screenshot();
            string fname;
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control))
            {
                var dlg = new SaveFileDialog()
                {
                    Filter = "PNG file (*.png)|*.png|JPEG file (*.jpg)|*.jpg|BMP file (*.bmp)|*.bmp",
                    Title = "保存屏幕截图",
                    AddExtension = true,
                    OverwritePrompt = true,
                    CheckPathExists = true,
                    ValidateNames = true,
                };
                if (dlg.ShowDialog() != true)
                    return;
                fname = dlg.FileName;
            }
            else
                fname = $"{DateTime.Now.ToString("yyyyMMdd-HHmmss")}.png";
            try
            {
                saver(fname);
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

        private void btnSave_Click(object sender, RoutedEventArgs e)
        {
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control))
            {
                var dlg = new OpenFileDialog()
                {
                    Filter = "xziar package (*.xzrp)|*.xzrp",
                    Title = "导入场景数据",
                    AddExtension = true,
                    CheckFileExists = true,
                    CheckPathExists = true,
                    Multiselect = false,
                    ValidateNames = true,
                };
                if (dlg.ShowDialog() != true)
                    return;
                try
                {
                    WaitingCount++;
                    Core.DeSerialize(dlg.FileName);
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
            else
            {
                var dlg = new SaveFileDialog()
                {
                    Filter = "xziar package (*.xzrp)|*.xzrp",
                    Title = "导出场景数据",
                    AddExtension = true,
                    OverwritePrompt = true,
                    CheckPathExists = true,
                    ValidateNames = true,
                };
                if (dlg.ShowDialog() != true)
                    return;
                try
                {
                    WaitingCount++;
                    Core.Serialize(dlg.FileName);
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
            glMain.Invalidate();
        }

        private async void btnTest_Click(object sender, RoutedEventArgs e)
        {
            var tex = ((sender as Button).DataContext as PBRMaterial).DiffuseMap;
            var img = await Core.ThumbMan.GetThumbnailAsync(tex);
            ((sender as Button).Content as Image).Source = img;
        }

        private async void AddShaderAsync(string fileName)
        {
            try
            {
                WaitingCount++;
                var shaderName = DateTime.Now.ToString("HH:mm:ss");
                var shader = await Core.LoadShaderAsync(fileName, shaderName);
                Core.Passes.Add(shader);
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
        }

        private void OnDropFile(object sender, System.Windows.Forms.DragEventArgs e)
        {
            string fname = (e.Data.GetData(System.Windows.Forms.DataFormats.FileDrop) as Array).GetValue(0).ToString();
            var extName = System.IO.Path.GetExtension(fname).ToLower();
            switch (extName)
            {
            case ".obj":
                AddModelAsync(fname);
                break;
            case ".glsl":
                AddShaderAsync(fname);
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
