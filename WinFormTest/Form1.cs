using System;
using OpenGLView;
using RayRender;
using System.Windows.Forms;

namespace WinFormTest
{
    public partial class Form1 : Form
    {
        private OGLView oglv;
        private BasicTest test;
        private bool isAnimate = false;
        private ushort curObj = 0;
        public Form1()
        {
            InitializeComponent();
            oglv = new OGLView()
            {
                Location = new System.Drawing.Point(0, 0),
                Name = "oglv",
                Size = ClientSize,
                ResizeBGDraw = false
            };
            test = new BasicTest();
            test.Resize((uint)ClientSize.Width & 0xffc0, (uint)ClientSize.Height & 0xffc0);

            Controls.Add(oglv);
            Resize += (o, e) => { oglv.Size = ClientSize; };
            FormClosing += (o, e) => { test.Dispose(); test = null; };

            oglv.Draw += test.Draw;
            oglv.Resize += (o, e) => { test.Resize((uint)e.Width & 0xffc0, (uint)e.Height & 0xffc0); };
            oglv.KeyDown += OnKeyDown;
            //oglv.KeyAction += OnKeyAction;
            oglv.MouseAction += OnMouse;
        }

        private void OnKeyDown(object sender, KeyEventArgs e)
        {
            Console.WriteLine($"KeyDown value[{e.KeyValue}] code[{e.KeyCode}] data[{e.KeyData}]");
            switch (e.KeyCode)
            {
                case Keys.Up:
                    test.Drawables[curObj].Move(0, 0.1f, 0); break;
                case Keys.Down:
                    test.Drawables[curObj].Move(0, -0.1f, 0); break;
                case Keys.Left:
                    test.Drawables[curObj].Move(-0.1f, 0, 0); break;
                case Keys.Right:
                    test.Drawables[curObj].Move(0.1f, 0, 0); break;
                case Keys.PageUp:
                    test.Drawables[curObj].Move(0, 0, -0.1f); break;
                case Keys.PageDown:
                    test.Drawables[curObj].Move(0, 0, 0.1f); break;
                case Keys.Add:
                    curObj++;
                    if (curObj >= test.Drawables.Size)
                        curObj = 0;
                    break;
                case Keys.Subtract:
                    if (curObj == 0)
                        curObj = (ushort)test.Drawables.Size;
                    curObj--;
                    break;
                default:
                    if (e.Shift)
                    {
                        switch (e.KeyValue)
                        {
                            case 'A':
                                test.Drawables[curObj].Rotate(0, 3, 0); break;
                            case 'D':
                                test.Drawables[curObj].Rotate(0, -3, 0); break;
                            case 'W':
                                test.Drawables[curObj].Rotate(3, 0, 0); break;
                            case 'S':
                                test.Drawables[curObj].Rotate(-3, 0, 0); break;
                            case 'Q':
                                test.Drawables[curObj].Rotate(0, 0, 3); break;
                            case 'E':
                                test.Drawables[curObj].Rotate(0, 0, -3); break;
                            case '\r':
                                isAnimate = !isAnimate; break;
                        }
                    }
                    else
                    {
                        switch (e.KeyValue)
                        {

                            case 'A'://pan to left
                                test.Camera.Yaw(3); break;
                            case 'D'://pan to right
                                test.Camera.Yaw(-3); break;
                            case 'W'://pan to up
                                test.Camera.Pitch(3); break;
                            case 'S'://pan to down
                                test.Camera.Pitch(-3); break;
                            case 'Q'://pan to left
                                test.Camera.Roll(-3); break;
                            case 'E'://pan to left
                                test.Camera.Roll(3); break;
                            case '\r':
                                test.Mode = !test.Mode; break;
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
                test.Camera.Move((e.dx * 10.0f / test.Width), (e.dy * 10.0f / test.Height), 0);
                break;
            case MouseEventType.Wheel:
                test.Camera.Move(0, 0, (float)e.dx);
                break;
            default:
                return;
            }
            oglv.Invalidate();
        }
    }
}
