using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace XZiar.Util
{
    public static class ReflectionHelper
    {
        public static T ToDelegate<T>(this MethodInfo method) where T : Delegate
        {
            return (T)method.CreateDelegate(typeof(T));
        }
    }
}
