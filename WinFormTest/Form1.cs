using System;
using OpenGLView;
using Dizz;
using System.Windows.Forms;
using System.Linq;

namespace WinFormTest
{
    public partial class Form1 : Form
    {
        //conver to radius
        const float MULER = (float)(Math.PI / 180);
        private OGLView GLView;
        private RenderCore Core = null;
        private bool IsAnimate = false;
        private IMovable[] OperateTargets = new IMovable[3];
        public Form1()
        {
            Common.ViewModelSyncRoot.Init();
            InitializeComponent();
            AllowDrop = true;
            DragEnter += (o, e) =>
            {
                if (e.Data.GetDataPresent(DataFormats.FileDrop))
                    e.Effect = DragDropEffects.Link;
                else
                    e.Effect = DragDropEffects.None;
            };
            DragDrop += OnDropFileAsync;

            GLView = new OGLView()
            {
                Location = new System.Drawing.Point(0, 0),
                Name = "oglv",
                Size = ClientSize,
                ResizeBGDraw = false
            };
            Core = new RenderCore();
            OperateTargets[0] = Core.TheScene.MainCamera;
            OperateTargets[1] = Core.TheScene.Drawables.LastOrDefault();
            OperateTargets[2] = Core.TheScene.Lights.LastOrDefault();
            Core.Resize((uint)ClientSize.Width, (uint)ClientSize.Height);

            Controls.Add(GLView);
            Resize += (o, e) => { GLView.Size = ClientSize; };
            FormClosing += (o, e) => { Core.Dispose(); Core = null; };

            GLView.Draw += Core.Draw;
            GLView.Resize += (o, e) => { Core.Resize((uint)e.Width, (uint)e.Height); };
            GLView.KeyDown += OnKeyDown;
            //oglv.KeyAction += OnKeyAction;
            GLView.MouseAction += OnMouse;
        }

        private async void OnDropFileAsync(object sender, DragEventArgs e)
        {
            string fname = (e.Data.GetData(DataFormats.FileDrop) as Array).GetValue(0).ToString();
            var extName = System.IO.Path.GetExtension(fname).ToLower();
            switch (extName)
            {
            case ".obj":
                {
                    var model = await Core.LoadModelAsync(fname);
                    model.Rotate(-90 * MULER, 0, 0);
                    model.Move(-1, 0, 0);
                    Core.TheScene.Drawables.Add(model);
                    OperateTargets[1] = Core.TheScene.Drawables.LastOrDefault();
                } break;
            }
            GLView.Invalidate();
        }

        private void OnKeyDown(object sender, KeyEventArgs e)
        {
            Console.WriteLine($"KeyDown value[{e.KeyValue}] code[{e.KeyCode}] data[{e.KeyData}]");
            switch (e.KeyCode)
            {
                case Keys.Up:
                    OperateTargets[1]?.Move(0, 0.1f, 0); break;
                case Keys.Down:
                    OperateTargets[1]?.Move(0, -0.1f, 0); break;
                case Keys.Left:
                    OperateTargets[1]?.Move(-0.1f, 0, 0); break;
                case Keys.Right:
                    OperateTargets[1]?.Move(0.1f, 0, 0); break;
                case Keys.PageUp:
                    OperateTargets[1]?.Move(0, 0, -0.1f); break;
                case Keys.PageDown:
                    OperateTargets[1]?.Move(0, 0, 0.1f); break;
                case Keys.Add:
                    {
                        var idx = Core.TheScene.Drawables.IndexOf(OperateTargets[1] as Drawable) + 1;
                        OperateTargets[1] = Core.TheScene.Drawables.ElementAtOrDefault(idx == Core.TheScene.Drawables.Count ? 0 : idx);
                    }
                    break;
                case Keys.Subtract:
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
                            case 'A':
                                OperateTargets[1]?.Rotate(0, 3 * MULER, 0); break;
                            case 'D':
                                OperateTargets[1]?.Rotate(0, -3 * MULER, 0); break;
                            case 'W':
                                OperateTargets[1]?.Rotate(3 * MULER, 0, 0); break;
                            case 'S':
                                OperateTargets[1]?.Rotate(-3 * MULER, 0, 0); break;
                            case 'Q':
                                OperateTargets[1]?.Rotate(0, 0, 3 * MULER); break;
                            case 'E':
                                OperateTargets[1]?.Rotate(0, 0, -3 * MULER); break;
                            case '\r':
                                IsAnimate = !IsAnimate; break;
                        }
                    }
                    else
                    {
                        switch (e.KeyValue)
                        {

                            case 'A'://pan to left
                                Core.TheScene.MainCamera.Yaw(3 * MULER); break;
                            case 'D'://pan to right
                                Core.TheScene.MainCamera.Yaw(-3 * MULER); break;
                            case 'W'://pan to up
                                Core.TheScene.MainCamera.Pitch(3 * MULER); break;
                            case 'S'://pan to down
                                Core.TheScene.MainCamera.Pitch(-3 * MULER); break;
                            case 'Q'://pan to left
                                Core.TheScene.MainCamera.Roll(-3 * MULER); break;
                            case 'E'://pan to left
                                Core.TheScene.MainCamera.Roll(3 * MULER); break;
                            case '\r':
                                /*Core.Mode = !Core.Mode;*/ break;
                        }
                    }
                    break;
            }
            (sender as OGLView).Invalidate();
        }

        private void OnMouse(object o, MouseEventExArgs e)
        {
            switch (e.Type)
            {
            case MouseEventType.Moving:
                Core.TheScene.MainCamera.Move((e.dx * 10.0f / Core.Width), (e.dy * 10.0f / Core.Height), 0);
                break;
            case MouseEventType.Wheel:
                Core.TheScene.MainCamera.Move(0, 0, (float)e.dx);
                break;
            default:
                return;
            }
            GLView.Invalidate();
        }
    }
}
