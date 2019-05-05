using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace AnyDock
{
    /// <summary>
    /// DraggingWindow.xaml 的交互逻辑
    /// </summary>
    internal partial class DraggingWindow : Window
    {
        private const int WM_MOVING = 0x0216;
        private const int WM_LBUTTONUP = 0x0202;
        private const int WM_NCLBUTTONUP = 0x00A2;
        private const int WM_EXITSIZEMOVE = 0x0232;

        public delegate void WindowDragEventHandler(DraggingWindow window, Point screenPos, DragData data);
        internal event WindowDragEventHandler Draging;
        internal event WindowDragEventHandler Draged;

        private readonly Point MouseDeltaPoint;
        internal readonly DragData Data;
        private bool HasInitialized = false;
        private bool BeginDragging = false;

        public DraggingWindow(Point startPoint, Point deltaPoint, DragData data)
        {
            Data = data;
            Width = Data.TabRoot.ActualWidth;
            Height = Data.TabRoot.ActualHeight;
            Left = startPoint.X; Top = startPoint.Y;
            MouseDeltaPoint = deltaPoint;
            InitializeComponent();
            RootTabs.TabStripPlacement = Data.TabRoot.TabStripPlacement;
            RootTabs.ShowIcon = Data.TabRoot.ShowIcon;
            RootTabs.RealChildren.Add(Data.Element);
        }

        protected override void OnContentRendered(EventArgs e)
        {
            base.OnContentRendered(e);
            if (!HasInitialized)
            {
                HasInitialized = true;
                CaptureMouse();
                var deltaPos = Mouse.GetPosition(this) - MouseDeltaPoint;
                ReleaseMouseCapture();
                Left += deltaPos.X; Top += deltaPos.Y;
                BeginDragging = true;
                if (Mouse.LeftButton == MouseButtonState.Pressed)
                {
                    Draging?.Invoke(this, GetMouseScreenPos(), Data);
                    DragMove(); // blocking call
                }
                FinishDrag();
            }
        }

        protected override void OnLocationChanged(EventArgs e)
        {
            base.OnLocationChanged(e);
            if (BeginDragging)
            {
                Draging?.Invoke(this, GetMouseScreenPos(), Data);
            }
        }

        private Point GetMouseScreenPos()
        {
            return PointToScreen(MouseDeltaPoint);
        }

        private void FinishDrag()
        {
            BeginDragging = false;
            var pos = GetMouseScreenPos();
            ((DraggableTabControl)Content).RealChildren.Clear();
            Close();
            Draged?.Invoke(this, pos, Data);
        }
    }
}
