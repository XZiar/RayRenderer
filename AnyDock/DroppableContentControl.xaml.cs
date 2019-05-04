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
        public DroppableContentControl()
        {
            InitializeComponent();
        }
        public bool ShowHeader { set => Header.Visibility = value ? Visibility.Visible : Visibility.Collapsed; }

        public virtual void OnDragDrop(DragData data, Point pos)
        {
            throw new NotImplementedException();
        }

        public virtual void OnDragIn(DragData data, Point pos)
        {
            throw new NotImplementedException();
        }

        public virtual void OnDragOut(DragData data, Point pos)
        {
            throw new NotImplementedException();
        }

        public virtual bool RecieveDrag()
        {
            throw new NotImplementedException();
        }

        private void BtnCloseClick(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)DataContext;
            var earg = new AnyDockManager.TabCloseEventArgs(element);
            element.RaiseEvent(earg);
            if (earg.ShouldClose)
            {
                earg.ChangeToClosed();
                element.RaiseEvent(earg);
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
