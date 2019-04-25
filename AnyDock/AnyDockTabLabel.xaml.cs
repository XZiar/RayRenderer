﻿using System;
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

namespace AnyDock
{
    /// <summary>
    /// AnyDockTabLabel.xaml 的交互逻辑
    /// </summary>
    public partial class AnyDockTabLabel : ContentControl
    {
        private AnyDockPanel ParentPanel = null;

        public static readonly DependencyProperty ParentTabProperty = DependencyProperty.Register(
            "ParentTab",
            typeof(TabControl),
            typeof(AnyDockTabLabel),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));
        public TabControl ParentTab { get => (TabControl)GetValue(ParentTabProperty); set => SetValue(ParentTabProperty, value); }


        public AnyDockTabLabel()
        {
            DataContextChanged += (o, e) =>
                {
                    ParentPanel = AnyDockManager.GetParentDock((UIElement)e.NewValue);
                };
            InitializeComponent();
        }
        
        //private void BeginTabItemDrag(MouseEventArgs e)
        //{
        //    AnyDockManager.PendingDrag.Source = null;
        //    DragDrop.DoDragDrop(this, new DragData(this), DragDropEffects.Move);
        //    if (AnyDockManager.PendingDrag.Source == this) // drag cancelled
        //    {
        //        AnyDockManager.PerformDrag(this);
        //    }
        //}
        //protected override void OnDragEnter(DragEventArgs e)
        //{
        //    base.OnDragEnter(e);
        //    var src = (DragData)e.Data.GetData(typeof(DragData));
        //    if (src != null)
        //    {
        //        if (src.Panel == ParentPanel || (src.AllowDrag && ParentPanel.AllowDropTab)) // self-reorder OR can dragdrop
        //        {
        //            e.Effects = DragDropEffects.Move;
        //            e.Handled = true;
        //            return;
        //        }
        //    }
        //    e.Effects = DragDropEffects.None;
        //}
        //protected override void OnDrop(DragEventArgs e)
        //{
        //    base.OnDrop(e);
        //    var src = (DragData)e.Data.GetData(typeof(DragData));
        //    if (src == null)
        //        return;
        //    var dst = new DragData(this);
        //    if (src.Panel == null || dst.Panel == null)
        //        throw new InvalidOperationException("Drag objects should belong to AnyDock");
        //    if (src.Panel == dst.Panel)
        //    {
        //        if (src.Element == dst.Element)
        //            return;
        //        // exchange order only
        //        int srcIdx = src.Panel.Children.IndexOf(src.Element);
        //        int dstIdx = dst.Panel.Children.IndexOf(dst.Element);
        //        src.Panel.Children.Move(srcIdx, dstIdx);
        //    }
        //    else
        //    {
        //        if (!src.AllowDrag || !ParentPanel.AllowDropTab) // only if can dragdrop
        //            return;
        //        if (src.Element == dst.Element)
        //            throw new InvalidOperationException("Should not be the same TabItem");
        //        // move item
        //        AnyDockPanel.DoDropTab(src, dst);
        //    }
        //    e.Handled = true;
        //}

        private void HandleClose(object sender, RoutedEventArgs e)
        {
            var element = (UIElement)DataContext;
            var earg = new AnyDockManager.TabClosingEventArgs(element, ParentPanel);
            element.RaiseEvent(earg);
            if (earg.ShouldClose)
                ParentPanel.Children.Remove(element);
        }
    }
}
