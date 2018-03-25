using System;
using System.Globalization;
using System.Windows.Data;

namespace XZiar.Util
{
    public static class BindingHelper
    {
        public delegate object ValueConvertDelegate(object value, Type targetType, object parameter, CultureInfo culture);
        public class OneWayValueConvertor : IValueConverter
        {
            private readonly ValueConvertDelegate Convertor;
            public OneWayValueConvertor(ValueConvertDelegate convertor) { Convertor = convertor; }
            public OneWayValueConvertor(Func<object, object> convertor) { Convertor = (o, t, p, c) => convertor(o); }
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                return Convertor(value, targetType, parameter, culture);
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }
        public class TwoWayValueConvertor : IValueConverter
        {
            private readonly ValueConvertDelegate Convertor;
            private readonly ValueConvertDelegate BackConvertor;
            public TwoWayValueConvertor(ValueConvertDelegate convertor, ValueConvertDelegate backConvertor) { Convertor = convertor; BackConvertor = backConvertor; }
            public TwoWayValueConvertor(Func<object, object> convertor, Func<object, object> backConvertor) { Convertor = (o, t, p, c) => convertor(o); BackConvertor = (o, t, p, c) => backConvertor(o); }
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                return Convertor(value, targetType, parameter, culture);
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                return BackConvertor(value, targetType, parameter, culture);
            }
        }
    }

}