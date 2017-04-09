using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using RayRender;

namespace WPFTest
{
    public static class Main
    {
        public enum OPObject
        { Camera, Light, Object };
        public static BasicTest test;
        private static ushort curObj_ = 0;
        public static ushort curObj
        {
            get { return curObj_; }
            set
            {
                if (value >= test.objectCount())
                    curObj_ = 0;
                else if (value == ushort.MaxValue)
                    curObj_ = (ushort)(test.objectCount() - 1);
                else
                    curObj_ = value;
            }
        }
        public static bool isAnimate = false;
        public static bool mode
        {
            get { return test.mode; }
            set { test.mode = value; }
        }

        static Main()
        {
            test = new BasicTest();
        }
        public static void Move(float x, float y, float z, OPObject obj)
        {
            if(obj == OPObject.Object)
            {
                test.moveobj((ushort)curObj, x, y, z);
            }
            else if(obj == OPObject.Camera)
            {
                test.cam.move(x, y, z);
            }
        }
        public static void Rotate(float x, float y, float z, OPObject obj)
        {
            if (obj == OPObject.Object)
            {
                test.rotateobj((ushort)curObj, x, y, z);
            }
            else if (obj == OPObject.Camera)
            {
                test.cam.pitch(x);
                test.cam.yaw(y);
                test.cam.roll(z);
            }
        }
    }
}
