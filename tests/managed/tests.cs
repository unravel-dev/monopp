using System;
using System.Runtime.CompilerServices;

namespace Tests
{

namespace Nested
{
class TestClassNested1
{
	class TestClassNested2
	{
		public int someField = 12;
	}
	public int someField = 12;
}	
}	

class MyObject
{
public
	MyObject()
	{
		CreateInternal(5.0f, "test");
	}

	~MyObject()
	{
		DestroyInternal();
	}

	[MethodImpl(MethodImplOptions.InternalCall)] 
	private extern void CreateInternal(float x, string s);

	[MethodImpl(MethodImplOptions.InternalCall)] 
	private extern void DestroyInternal();

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern void DoStuff(string value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern string ReturnAString(string value);
}

class MonoppTest
{

    public int someField = 12;

    public int someProperty
	{
		get
		{
			return someField;
		}
		set
		{
            Console.WriteLine("FROM C# : Setting property value to {0}", value);
			someField = value;
		}
	}

    public static int someFieldStatic = 12;

    public static int somePropertyStatic
	{
		get
		{
			return someFieldStatic;
		}
		set
		{
			//Console.WriteLine("FROM C# : Setting static property value to {0}", value);
			someFieldStatic = value;
		}
	}
	
    static MonoppTest()
	{
		//Console.WriteLine("FROM C# : STATIC CONSTRUCTOR.");
	}
    public MonoppTest()
	{
		//Console.WriteLine("FROM C# : MonoppTest created.");
	}
    ~MonoppTest()
	{
		//Console.WriteLine("FROM C# : MonoppTest destroyed.");
	}

    void Method1()
	{
        //Console.WriteLine("FROM C# : Hello Mono World from instance.");
	}
	
	void Method2(string s)
	{
		//Console.WriteLine("FROM C# : WithParam string: " + s);
	}
	void Method3(int s)
	{
		//Console.WriteLine("FROM C# : WithParam int: " + s);
	}
	void Method4(int s, int s1)
	{
		//Console.WriteLine("FROM C# : WithParam int, int: {0}, {1}", s, s1);
	}
	
    public string Method5(string s, int b)
	{
		//Console.WriteLine("FROM C# : WithParam: {0}, {1}", s, b);
		return "Return Value: " + s;
	}

    public static int Function1(int a)
	{
		//Console.WriteLine("FROM C# : Int value: " + a);
		return a + 1337;
	}

    public static void Function2(float a, int b, float c)
	{
		//Console.WriteLine("FROM C# : VoidMethod: {0}, {1}, {2}", a, b, c);
	}

    public static void Function3(string a)
	{
		//Console.WriteLine("FROM C# : String value: {0}", a);
	}

    public static string Function4(string str)
	{
		return "The string value was: " + str;
	}
	
    public static void Function5()
	{
		throw new Exception("Hello!");
	}
	
    public static void Function6()
	{
		Tests.MyObject obj = new Tests.MyObject();
		obj.DoStuff("blalba");
		string str = obj.ReturnAString("fafafa");
        str += "";
		//Console.WriteLine("FROM C# : ReturnAString : {0}", str);
	}
}


public struct Vector2f  
{
    public Vector2f(float _x, float _y)
	{
		x = _x;
		y = _y;
	}
    public float x;
    public float y;
}

public class WrapperVector2f : Monopp.Core.NativeObject
{
	[MethodImpl(MethodImplOptions.InternalCall)] 
	public extern WrapperVector2f(float x, float y);

	[MethodImpl(MethodImplOptions.InternalCall)] 
	public extern WrapperVector2f(WrapperVector2f rhs);

    public void Foo()
	{
	}
}

class MonortTest
{
    [MethodImpl(MethodImplOptions.InternalCall)] 
	public extern void TestInternalPODCall(Vector2f rhs);
    
    public Vector2f someFieldPOD = new Vector2f(12, 13);

    public Vector2f somePropertyPOD
	{
		get
		{
			return someFieldPOD;
		}
		set
		{
			//Console.WriteLine("FROM C# : Setting POD property value to {0}, {1}", value.x, value.y);
			someFieldPOD = value;
		}
	}

    public static Vector2f someFieldPODStatic = new Vector2f(12, 13);

    public static Vector2f somePropertyPODStatic
	{
		get
		{
			return someFieldPODStatic;
		}
		set
		{
			//Console.WriteLine("FROM C# : Setting static POD property value to {0}, {1}", value.x, value.y);
			someFieldPODStatic = value;
		}
	}

    public WrapperVector2f someFieldNONPOD = new WrapperVector2f(12, 13);

    public WrapperVector2f somePropertyNONPOD
	{
		get
		{
			return someFieldNONPOD;
		}
		set
		{
			//Console.WriteLine("FROM C# : Setting NON POD property value");
			someFieldNONPOD = value;
		}
	}

public
	static WrapperVector2f someFieldNONPODStatic = new WrapperVector2f(12, 13);

public
	static WrapperVector2f somePropertyNONPODStatic
	{
		get
		{
			return someFieldNONPODStatic;
		}
		set
        {
			//Console.WriteLine("FROM C# : Setting static NON POD property value");
			someFieldNONPODStatic = value;
		}
	}
	
	public Vector2f MethodPodAR(Vector2f bb)
	{
		//Console.WriteLine(bb.x);
		//Console.WriteLine(bb.y);
		var s = new Vector2f();
		s.x = 165.0f;
		s.y = 7.0f;

		return s;
	}

	public WrapperVector2f MethodPodARW(WrapperVector2f bb)
	{
		//Console.WriteLine("FROM C# :");
		var s = new WrapperVector2f(55.0f, 66.0f);
		var s1 = new WrapperVector2f(s);

		s.Foo();
		s1.Foo();
        TestInternalPODCall(new Vector2f(5, 12));
		return s;
	}
}

} // namespace Tests

