using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Windows.Data;

namespace XZiar.Util
{
    public class ArrayIndexMultiConverter : IMultiValueConverter
    {
        public object Convert(object[] value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value == null)
                return null;
            if (value.Length != 2)
                throw new ArgumentException("need two binding point");
            var arr = value[0] as IEnumerable<object>;
            var idx = value[1] as int?;
            if (arr == null)
                return null;
            if (idx == null)
                throw new ArgumentException("need an int as index");
            return arr.ElementAt(idx.Value);
        }
        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ArrayIndexConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            var idx = parameter as int?;
            if (idx == null)
                throw new ArgumentException("need an int as index");
            var arr = value as IEnumerable<object>;
            return arr?.ElementAt(idx.Value);
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

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
            public static TwoWayValueConvertor DefaultConvertor = new TwoWayValueConvertor
                ((o, t, p, c) => System.Convert.ChangeType(o, t), (o, t, p, c) => System.Convert.ChangeType(o, t));
            public static TwoWayValueConvertor From<T>(Func<object, T> convertor)
            {
                return new TwoWayValueConvertor((o, t, p, c) => convertor(o) as object, (o, t, p, c) => convertor(o) as object);
            }
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

        public delegate object MultiValueConvertDelegate(object[] value, Type targetType, object parameter, CultureInfo culture);
        public delegate object[] MultiValueConvertBackDelegate(object value, Type[] targetType, object parameter, CultureInfo culture);
        public class OneWayMultiValueConvertor : IMultiValueConverter
        {
            private readonly MultiValueConvertDelegate Convertor;
            public OneWayMultiValueConvertor(MultiValueConvertDelegate convertor) { Convertor = convertor; }
            public OneWayMultiValueConvertor(Func<object[], object> convertor) { Convertor = (o, t, p, c) => convertor(o); }
            public object Convert(object[] value, Type targetType, object parameter, CultureInfo culture)
            {
                return Convertor(value, targetType, parameter, culture);
            }
            public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }
        public class TwoWayMultiValueConvertor : IMultiValueConverter
        {
            private readonly MultiValueConvertDelegate Convertor;
            private readonly MultiValueConvertBackDelegate BackConvertor;
            public TwoWayMultiValueConvertor(MultiValueConvertDelegate convertor, MultiValueConvertBackDelegate backConvertor) { Convertor = convertor; BackConvertor = backConvertor; }
            public TwoWayMultiValueConvertor(Func<object[], object> convertor, Func<object, object[]> backConvertor) { Convertor = (o, t, p, c) => convertor(o); BackConvertor = (o, t, p, c) => backConvertor(o); }
            public object Convert(object[] value, Type targetType, object parameter, CultureInfo culture)
            {
                return Convertor(value, targetType, parameter, culture);
            }
            public object[] ConvertBack(object value, Type[] targetType, object parameter, CultureInfo culture)
            {
                return BackConvertor(value, targetType, parameter, culture);
            }
        }
    }

}