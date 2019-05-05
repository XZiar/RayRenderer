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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace AnyDock
{
    /// <summary>
    /// DroppableContentControl.xaml 的交互逻辑
    /// </summary>
    internal partial class DroppableContentControl : ContentControl, IDragRecievePoint
    {
        internal static readonly DependencyProperty ParentTabProperty = DependencyProperty.Register(
            "ParentTab",
            typeof(DraggableTabControl),
            typeof(DroppableContentControl),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));
        internal DraggableTabControl ParentTab
        {
            get => (DraggableTabControl)GetValue(ParentTabProperty);
            set => SetValue(ParentTabProperty, value);
        }
        private readonly Viewbox DragOverLay;

        static DroppableContentControl()
        {
            EventManager.RegisterClassHandler(typeof(DroppableContentControl), DragManager.RecieveDragEvent, new DragManager.RecieveDragEventHandler(OnRecieveDrag));
        }

        public DroppableContentControl()
        {
            InitializeComponent();
            DragOverLay = (Viewbox)FindResource("DragOverLay");
        }

        public bool ShowHeader { set => Header.Visibility = value ? Visibility.Visible : Visibility.Collapsed; }

        private static void OnRecieveDrag(UIElement sender, DragManager.RecieveDragEventArgs e)
        {
            var self = (DroppableContentControl)sender;
            if (self.ParentTab?.AllowDropTab ?? false)
            {
                e.RecievePoint = self;
                e.Handled = true;
            }
        }

        public virtual void OnDragDrop(DragData data, Point pos)
        {
            throw new NotImplementedException();
        }

        public virtual void OnDragIn(DragData data, Point pos)
        {
            MainGrid.Children.Add(DragOverLay);
        }

        public virtual void OnDragOut(DragData data, Point pos)
        {
            MainGrid.Children.Remove(DragOverLay);
        }

        private void BtnCloseClick(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)DataContext;
            var earg = new AnyDockManager.TabCloseEventArgs(element);
            element.RaiseEvent(earg);
            if (earg.ShouldClose)
            {
                AnyDockManager.RaiseRemovedEvent(element);
            }
        }

        private void BtnPinClick(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)DataContext;
            var isCollapsed = AnyDockSidePanel.GetCollapseToSide(element);
            element.SetValue(AnyDockSidePanel.CollapseToSideProperty, !isCollapsed);
        }
    }
}
