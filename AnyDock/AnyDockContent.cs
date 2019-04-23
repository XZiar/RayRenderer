using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;

namespace AnyDock
{
    class AnyDockContent : ContentControl
    {
        private AnyDockPanel ParentPanel = null;

        public AnyDockContent() : base() { AllowDrop = true; }

        protected override void OnContentChanged(object oldContent, object newContent)
        {
            base.OnContentChanged(oldContent, newContent);
            ParentPanel = AnyDockManager.GetParentDock((UIElement)newContent);
            SetBinding(VisibilityProperty, new Binding { Path = new PropertyPath(AnyDockPanel.IsHiddenProperty), Source = ParentPanel, Converter = new CollapseIfTrueConverter() });
        }

        protected override void OnDragEnter(DragEventArgs e)
        {
            var src = (DragData)e.Data.GetData(typeof(DragData));
            if (src != null && src.AllowDrag && ParentPanel.AllowDropTab) // only if can dragdrop
            {
                e.Effects = DragDropEffects.Move;
                e.Handled = true;
                ParentPanel.OnContentDragEnter(this);
            }
            else
                e.Effects = DragDropEffects.None;
            base.OnDragEnter(e);
        }

        protected override void OnDragLeave(DragEventArgs e)
        {
            if (e.Data.GetDataPresent(typeof(DragData)))
            {
                e.Handled = true;
                ParentPanel.OnContentDragLeave(this);
            }
            base.OnDragLeave(e);
        }

        protected override void OnDrop(DragEventArgs e)
        {
            var src = (DragData)e.Data.GetData(typeof(DragData));
            if (src != null && src.AllowDrag && ParentPanel.AllowDropTab) // only if can dragdrop
            {
                e.Handled = true;
                ParentPanel.OnContentDrop(this, src, e);
            }
            base.OnDrop(e);
        }


    }
}
