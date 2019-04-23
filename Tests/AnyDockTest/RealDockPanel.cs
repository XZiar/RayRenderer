using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Shapes;

namespace AnyDockTest
{
    [ContentProperty(nameof(Child))]
    class RealDockPanel : Panel
    {
        private Rectangle RectTop = new Rectangle { Width=500, Height=500 };
        private Rectangle RectBottom = new Rectangle { Width=500, Height=500 };
        private Rectangle RectLeft = new Rectangle { Width=500, Height=500 };
        private Rectangle RectRight = new Rectangle { Width=500, Height=500 };
        private UIElement child;
        public UIElement Child { get { return child; } set { Children.Remove(child); Children.Add(value); child = value; } }
        public Brush Top { get { return RectTop.Fill; } set { RectTop.Fill = value; } }
        public Brush Bottom { get { return RectBottom.Fill; } set { RectBottom.Fill = value; } }
        public Brush Left { get { return RectLeft.Fill; } set { RectLeft.Fill = value; } }
        public Brush Right { get { return RectRight.Fill; } set { RectRight.Fill = value; } }

        private double LenTop = 10, LenBottom = 10, LenLeft = 10, LenRight = 10;
        public RealDockPanel()
        {
            Children.Add(RectTop);
            Children.Add(RectBottom);
            Children.Add(RectLeft);
            Children.Add(RectRight);
        }
        protected override Size MeasureOverride(Size availableSize)
        {
            if (RectLeft != null)
            {
                var thisSize = new Size{ Width=LenLeft, Height = availableSize.Height};
                RectLeft.Measure(thisSize);
            }
            if (RectRight != null)
            {
                var thisSize = new Size{ Width=LenRight, Height = availableSize.Height};
                RectRight.Measure(thisSize);
            }
            if (RectTop != null)
            {
                var thisSize = new Size{ Width=availableSize.Width - LenLeft - LenRight, Height = LenTop};
                RectTop.Measure(thisSize);
            }
            if (RectBottom != null)
            {
                var thisSize = new Size{ Width=availableSize.Width - LenLeft - LenRight, Height = LenBottom};
                RectBottom.Measure(thisSize);
            }
            if (Child != null)
            {
                var thisSize = new Size{ Width=availableSize.Width - LenLeft - LenRight, Height = availableSize.Height - LenTop - LenBottom};
                Child.Measure(thisSize);
            }
            return availableSize;
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            if (RectLeft != null)
            {
                RectLeft.Arrange(new Rect(0, 0, RectLeft.DesiredSize.Width, RectLeft.DesiredSize.Height));
            }
            if (RectRight != null)
            {
                RectRight.Arrange(new Rect(finalSize.Width - LenRight, 0, RectRight.DesiredSize.Width, RectRight.DesiredSize.Height));
            }
            if (RectTop != null)
            {
                RectTop.Arrange(new Rect(LenLeft, 0, RectTop.DesiredSize.Width, RectTop.DesiredSize.Height));
            }
            if (RectBottom != null)
            {
                RectBottom.Arrange(new Rect(LenLeft, finalSize.Height - LenBottom, RectBottom.DesiredSize.Width, RectBottom.DesiredSize.Height));
            }
            if (Child != null)
            {
                Child.Arrange(new Rect(LenLeft, LenTop, finalSize.Width - LenLeft - LenRight, finalSize.Height - LenTop - LenBottom));
            }
            return finalSize;
        }
    }
}
