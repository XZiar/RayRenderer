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
        private UInt16 curObj = 0;
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
            test.Resize(ClientSize.Width & 0xffc0, ClientSize.Height & 0xffc0);

            Controls.Add(oglv);
            Resize += (o, e) => { oglv.Size = ClientSize; };
            FormClosing += (o, e) => { test.Dispose(); test = null; };

            oglv.Draw += test.Draw;
            oglv.Resize += (o, e) => { test.Resize(e.Width & 0xffc0, e.Height & 0xffc0); };
            oglv.KeyAction += OnKeyAction;
            oglv.MouseAction += OnMouse;
        }

        private void OnKeyAction(object o, KeyBoardEventArgs e)
        {
            Console.WriteLine($"KeyAction {e.key}");
            switch (e.SpecialKey())
            {
            case Key.Up:
                test.Moveobj(curObj, 0, 0.1f, 0); break;
            case Key.Down:
                test.Moveobj(curObj, 0, -0.1f, 0); break;
            case Key.Left:
                test.Moveobj(curObj, -0.1f, 0, 0); break;
            case Key.Right:
                test.Moveobj(curObj, 0.1f, 0, 0); break;
            case Key.PageUp:
                test.Moveobj(curObj, 0, 0, -0.1f); break;
            case Key.PageDown:
                test.Moveobj(curObj, 0, 0, 0.1f); break;
            default:
                switch (e.key)
                {
                case 'a'://pan to left
                    test.cam.Yaw(3); break;
                case 'd'://pan to right
                    test.cam.Yaw(-3); break;
                case 'w'://pan to up
                    test.cam.Pitch(3); break;
                case 's'://pan to down
                    test.cam.Pitch(-3); break;
                case 'q'://pan to left
                    test.cam.Roll(-3); break;
                case 'e'://pan to left
                    test.cam.Roll(3); break;
                case 'A':
                    test.Rotateobj(curObj, 0, 3, 0); break;
                case 'D':
                    test.Rotateobj(curObj, 0, -3, 0); break;
                case 'W':
                    test.Rotateobj(curObj, 3, 0, 0); break;
                case 'S':
                    test.Rotateobj(curObj, -3, 0, 0); break;
                case 'Q':
                    test.Rotateobj(curObj, 0, 0, 3); break;
                case 'E':
                    test.Rotateobj(curObj, 0, 0, -3); break;
                case (char)13:
                    if(e.hasShift())
                        isAnimate = !isAnimate;
                    else
                        test.mode = !test.mode;
                    break;
                default:
                    break;
                case '+':
                    curObj++;
                    if (curObj >= test.objectCount)
                        curObj = 0;
                    //test.showObject(curObj);
                    break;
                case '-':
                    if (curObj == 0)
                        curObj = test.objectCount;
                    curObj--;
                    //test.showObject(curObj);
                    break;
                }
                break;
            }
            oglv.Invalidate();
        }

        private void OnMouse(object o, MouseEventExArgs e)
        {
            switch (e.type)
            {
            case MouseEventType.Moving:
                test.cam.Move((e.dx * 10.0f / test.cam.Width), (e.dy * 10.0f / test.cam.Height), 0);
                break;
            case MouseEventType.Wheel:
                test.cam.Move(0, 0, (float)e.dx);
                break;
            default:
                return;
            }
            oglv.Invalidate();
        }
    }
}
