using System;
using System.Collections.Generic;
using System.ComponentModel;
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
using XZiar.Util;
using static AnyDock.AnyDockManager;

namespace AnyDock
{
    internal delegate void DropTabEventHandler(DragData src, DragData dst);
    

    /// <summary>
    /// AnyDockTabLabel.xaml 的交互逻辑
    /// </summary>
    public partial class AnyDockTabLabel : UserControl
    {

        private AnyDockPanel ParentPanel = null;
        private static readonly PropertyPath TabStripPath = new PropertyPath(TabControl.TabStripPlacementProperty);
        private static readonly IValueConverter TabStripConvertor = new BindingHelper.OneWayValueConvertor(o => 
        {
            switch ((Dock)o)
            {
            case Dock.Left:  return 270.0;
            case Dock.Right: return 90.0;
            default:         return 0.0;
            }
        });
        private static readonly FrameworkPropertyMetadata ParentTabMeta = new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender,
            new PropertyChangedCallback((o,e) =>
            {
                BindingOperations.SetBinding(((AnyDockTabLabel)o).LayoutTransform, RotateTransform.AngleProperty, 
                    new Binding { Source=(TabControl)e.NewValue, Path=TabStripPath, Converter=TabStripConvertor });
            }));
        public static readonly DependencyProperty ParentTabProperty = DependencyProperty.Register(
            "ParentTab",
            typeof(TabControl),
            typeof(AnyDockTabLabel),
            ParentTabMeta);
        private class DragInfo { public AnyDockTabLabel Source = null; public Point StarPoint; }
        private static readonly DragInfo PendingDrag = new DragInfo();

        internal static event DropTabEventHandler DropTab;

        public TabControl ParentTab { get => (TabControl)GetValue(ParentTabProperty); set => SetValue(ParentTabProperty, value); }
        public AnyDockTabLabel()
        {
            LayoutTransform = new RotateTransform();
            DataContextChanged += (o, e) =>
                {
                    ParentPanel = AnyDockPanel.GetParentDock((UIElement)e.NewValue);
                };
            InitializeComponent();
        }

        
        protected override void OnMouseLeftButtonDown(MouseButtonEventArgs e)
        {
            base.OnMouseLeftButtonDown(e);
            PendingDrag.Source = this;
            PendingDrag.StarPoint = e.GetPosition(null);
        }
        protected override void OnMouseLeave(MouseEventArgs e)
        {
            base.OnMouseLeave(e);
            if (e.LeftButton == MouseButtonState.Pressed && PendingDrag.Source == this)
            {
                BeginTabItemDrag();
                e.Handled = true;
            }
        }
        protected override void OnMouseMove(MouseEventArgs e)
        {
            base.OnMouseMove(e);
            if (e.LeftButton == MouseButtonState.Pressed && PendingDrag.Source == this)
            {
                var diff = e.GetPosition(null) - PendingDrag.StarPoint;
                if (Math.Abs(diff.X) >= SystemParameters.MinimumHorizontalDragDistance &&
                    Math.Abs(diff.Y) >= SystemParameters.MinimumVerticalDragDistance)
                {
                    BeginTabItemDrag();
                    e.Handled = true;
                }
            }
        }
        private void BeginTabItemDrag()
        {
            PendingDrag.Source = null;
            DragDrop.DoDragDrop(this, new DragData(this), DragDropEffects.Move);
        }
        protected override void OnDragEnter(DragEventArgs e)
        {
            base.OnDragEnter(e);
            var src = (DragData)e.Data.GetData(typeof(DragData));
            if (src != null)
            {
                if (src.Panel == ParentPanel || (src.AllowDrag && ParentPanel.AllowDropTab)) // self-reorder OR can dragdrop
                {
                    e.Effects = DragDropEffects.Move;
                    e.Handled = true;
                    return;
                }
            }
            e.Effects = DragDropEffects.None;
        }
        protected override void OnDrop(DragEventArgs e)
        {
            base.OnDrop(e);
            var src = (DragData)e.Data.GetData(typeof(DragData));
            if (src == null)
                return;
            var dst = new DragData(this);
            if (src.Panel == null || dst.Panel == null)
                throw new InvalidOperationException("Drag objects should belong to AnyDock");
            if (src.Panel == dst.Panel)
            {
                if (src.Element == dst.Element)
                    return;
                // exchange order only
                int srcIdx = src.Panel.Children.IndexOf(src.Element);
                int dstIdx = dst.Panel.Children.IndexOf(dst.Element);
                src.Panel.Children.Move(srcIdx, dstIdx);
            }
            else
            {
                if (!src.AllowDrag || !ParentPanel.AllowDropTab) // only if can dragdrop
                    return;
                if (src.Element == dst.Element)
                    throw new InvalidOperationException("Should not be the same TabItem");
                // move item
                DropTab(src, dst);
            }
            e.Handled = true;
        }

        private void HandleClose(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)DataContext;
            var earg = new TabClosingEventArgs(element, ParentPanel);
            element.RaiseEvent(earg);
            if (earg.ShouldClose)
                ParentPanel.Children.Remove(element);
        }
    }
}
