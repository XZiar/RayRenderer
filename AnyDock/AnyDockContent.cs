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
    class AnyDockContent : ContentControl, IDragRecievePoint
    {
        public AnyDockContent()
        {
        }

        public AnyDockPanel ParentDockPoint { get; private set; }

        protected override void OnContentChanged(object oldContent, object newContent)
        {
            base.OnContentChanged(oldContent, newContent);
            ParentDockPoint = AnyDockManager.GetParentDock((UIElement)newContent);
        }

        public virtual void OnDragIn(DragData data, Point pos)
        {
            ParentDockPoint.OnContentDragEnter(this);
        }

        public virtual void OnDragOut(DragData data, Point pos)
        {
            ParentDockPoint.OnContentDragLeave(this);
        }

        public virtual void OnDragDrop(DragData data, Point pos)
        {
            ParentDockPoint.OnContentDrop(this, data, PointToScreen(pos));
        }

    }
}
