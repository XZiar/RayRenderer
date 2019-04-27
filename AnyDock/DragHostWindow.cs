using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Interop;

namespace AnyDock
{
    class DragHostWindow : Window
    {
        private const int WM_MOVING = 0x0216;
        private const int WM_LBUTTONUP = 0x0202;
        private const int WM_NCLBUTTONUP = 0x00A2;
        private const int WM_EXITSIZEMOVE = 0x0232;

        public delegate void WindowDragEventHandler(DragHostWindow window, Point screenPos, DragData data);
        internal event WindowDragEventHandler Draging;
        internal event WindowDragEventHandler Draged;

        //private HwndSource WindowHandle;
        private readonly Point MouseDeltaPoint;
        internal readonly DragData Data;
        public DragHostWindow(Point startPoint, Point deltaPoint, DragData data)
        {
            Data = data;
            WindowStyle = WindowStyle.None;
            AllowsTransparency = true;
            IsHitTestVisible = false;
            ShowInTaskbar = false;
            ShowActivated = false;
            Focusable = false;
            FocusManager.SetIsFocusScope(this, false);
            ResizeMode = ResizeMode.NoResize;
            SizeToContent = SizeToContent.Manual;
            Width = Data.Panel.ActualWidth;
            Height = Data.Panel.ActualHeight;
            //SizeToContent = SizeToContent.WidthAndHeight;
            WindowStartupLocation = WindowStartupLocation.Manual;
            Left = startPoint.X; Top = startPoint.Y;
            MouseDeltaPoint = deltaPoint;
            ContentRendered += ContentFirstRendered;

            var panel = new AnyDockPanel(false)
            {
                TabStripPlacement = Data.Panel.TabStripPlacement
            };
            panel.Children.Add(Data.Element);
            Content = panel;
        }

        private void ContentFirstRendered(object sender, EventArgs e)
        {
            ContentRendered -= ContentFirstRendered;
            CaptureMouse();
            var deltaPos = Mouse.GetPosition(this) - MouseDeltaPoint;
            ReleaseMouseCapture();
            Left += deltaPos.X; Top += deltaPos.Y;
            //WindowHandle = HwndSource.FromHwnd(new WindowInteropHelper(this).Handle);
            //WindowHandle.AddHook(HookWindowProc);
            LocationChanged += OnDragWindow;
            if (Mouse.LeftButton == MouseButtonState.Pressed)
            {
                Draging(this, GetMouseScreenPos(), Data);
                DragMove(); // blocking call
            }
            FinishDrag();
        }

        private Point GetMouseScreenPos()
        {
            //var pos = Mouse.GetPosition(this);
            //if (pos != MouseDeltaPos)
            //    Console.WriteLine($"Mismatch in [{MouseDeltaPos}] and [{pos}]");
            return PointToScreen(MouseDeltaPoint);
        }

        private void OnDragWindow(object sender, EventArgs e)
        {
            Draging(this, GetMouseScreenPos(), Data);
        }

        private IntPtr HookWindowProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            switch (msg)
            {
            case WM_EXITSIZEMOVE:
                FinishDrag();
                break;
            }
            return IntPtr.Zero;
        }

        private void FinishDrag()
        {
            LocationChanged -= OnDragWindow;
            //WindowHandle.RemoveHook(HookWindowProc);
            var pos = GetMouseScreenPos();
            ((AnyDockPanel)Content).Children.Clear();
            Close();
            Draged(this, pos, Data);
        }

    }
}
