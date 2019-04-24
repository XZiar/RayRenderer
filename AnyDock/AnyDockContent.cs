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

        public AnyDockContent()
        {
            AllowDrop = true;
        }

        protected override void OnContentChanged(object oldContent, object newContent)
        {
            base.OnContentChanged(oldContent, newContent);
            ParentPanel = AnyDockManager.GetParentDock((UIElement)newContent);
        }

        protected override void OnDragEnter(DragEventArgs e)
        {
            var src = (DragData)e.Data.GetData(typeof(DragData));
            if (src != null)
            {
                if (!src.AllowDrag || !ParentPanel.AllowDropTab)
                {
                    e.Effects = DragDropEffects.None;
                }
                else // only if can dragdrop
                {
                    e.Effects = DragDropEffects.Move;
                    ParentPanel.OnContentDragEnter(this);
                }
                e.Handled = true;
            }
            else
            {
                base.OnDragEnter(e);
            }
        }

        protected override void OnDragLeave(DragEventArgs e)
        {
            if (e.Data.GetDataPresent(typeof(DragData)))
            {
                e.Handled = true;
                ParentPanel.OnContentDragLeave(this);
            }
            else
            {
                base.OnDragLeave(e);
            }
        }

        protected override void OnDrop(DragEventArgs e)
        {
            var src = (DragData)e.Data.GetData(typeof(DragData));
            if (src != null)
            {
                if (src.AllowDrag && ParentPanel.AllowDropTab) // only if can dragdrop
                    ParentPanel.OnContentDrop(this, src, e);
                e.Handled = true;
                return;
            }
            base.OnDrop(e);
        }


    }
}
