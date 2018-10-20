﻿using System;
using OpenGLView;
using Dizz;
using System.Windows.Forms;

namespace WinFormTest
{
    public partial class Form1 : Form
    {
        private OGLView GLView;
        private RenderCore Core;
        private bool IsAnimate = false;
        private ushort curObj = 0;
        public Form1()
        {
            InitializeComponent();
            GLView = new OGLView()
            {
                Location = new System.Drawing.Point(0, 0),
                Name = "oglv",
                Size = ClientSize,
                ResizeBGDraw = false
            };
            Core = new RenderCore();
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

        private void OnKeyDown(object sender, KeyEventArgs e)
        {
            Console.WriteLine($"KeyDown value[{e.KeyValue}] code[{e.KeyCode}] data[{e.KeyData}]");
            switch (e.KeyCode)
            {
                case Keys.Up:
                    Core.TheScene.Drawables[curObj].Move(0, 0.1f, 0); break;
                case Keys.Down:
                    Core.TheScene.Drawables[curObj].Move(0, -0.1f, 0); break;
                case Keys.Left:
                    Core.TheScene.Drawables[curObj].Move(-0.1f, 0, 0); break;
                case Keys.Right:
                    Core.TheScene.Drawables[curObj].Move(0.1f, 0, 0); break;
                case Keys.PageUp:
                    Core.TheScene.Drawables[curObj].Move(0, 0, -0.1f); break;
                case Keys.PageDown:
                    Core.TheScene.Drawables[curObj].Move(0, 0, 0.1f); break;
                case Keys.Add:
                    curObj++;
                    if (curObj >= Core.TheScene.Drawables.Count)
                        curObj = 0;
                    break;
                case Keys.Subtract:
                    if (curObj == 0)
                        curObj = (ushort)Core.TheScene.Drawables.Count;
                    curObj--;
                    break;
                default:
                    if (e.Shift)
                    {
                        switch (e.KeyValue)
                        {
                            case 'A':
                                Core.TheScene.Drawables[curObj].Rotate(0, 3, 0); break;
                            case 'D':
                                Core.TheScene.Drawables[curObj].Rotate(0, -3, 0); break;
                            case 'W':
                                Core.TheScene.Drawables[curObj].Rotate(3, 0, 0); break;
                            case 'S':
                                Core.TheScene.Drawables[curObj].Rotate(-3, 0, 0); break;
                            case 'Q':
                                Core.TheScene.Drawables[curObj].Rotate(0, 0, 3); break;
                            case 'E':
                                Core.TheScene.Drawables[curObj].Rotate(0, 0, -3); break;
                            case '\r':
                                IsAnimate = !IsAnimate; break;
                        }
                    }
                    else
                    {
                        switch (e.KeyValue)
                        {

                            case 'A'://pan to left
                                Core.TheScene.MainCamera.Yaw(3); break;
                            case 'D'://pan to right
                                Core.TheScene.MainCamera.Yaw(-3); break;
                            case 'W'://pan to up
                                Core.TheScene.MainCamera.Pitch(3); break;
                            case 'S'://pan to down
                                Core.TheScene.MainCamera.Pitch(-3); break;
                            case 'Q'://pan to left
                                Core.TheScene.MainCamera.Roll(-3); break;
                            case 'E'://pan to left
                                Core.TheScene.MainCamera.Roll(3); break;
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
