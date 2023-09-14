R"=====(package fos;

import openfl.utils.ByteArray;
import openfl.utils.Endian;
import openfl.utils.IDataInput;
import openfl.utils.IDataOutput;

class FosUtils
{
    
    public static function WriteString(output : IDataOutput, string : String) : Void
    {
        var tmp : ByteArray = new ByteArray();
        tmp.endian = Endian.LITTLE_ENDIAN;
        tmp.writeUTFBytes(string);
        WriteLEB128_u32(output, tmp.length);
        output.writeBytes(tmp);
    }
    
    public static function ReadString(input : IDataInput) : String
    {
        var length : Int = cast((input), ReadLEB128_u32);
        return input.readUTFBytes(length);
    }
    
    public static function ReadLEB128_32(input : IDataInput) : Int
    {
        var result : Int = 0;
        var shift : Int = 0;
        var size : Int = 32;
        while (true)
        {
            if (input.bytesAvailable < 1)
            {
                return 0;
            }
            var byte : Int = input.readByte();
            
            result = result | Int( (byte & 0x7F) << shift );
            shift += 7;
            /* sign bit of byte is second high order bit (0x40) */
            if ((byte & 0x80) == 0)
            {
                break;
            }
        }
        //if left something to fill for negative value:
        if ((shift < size) && ((result & Int( 1 << (shift - 1) )) != 0)) {
            result = result | -(1 << shift);
        }
        
        return result;
    }
    
    public static function ReadLEB128_u32(input : IDataInput) : Int
    {
        var result : Int = 0;
        var shift : Int = 0;
        while (true)
        {
            if (input.bytesAvailable < 1)
            {
                return 0;
            }
            var byte : Int = input.readByte();
            
            result = result | Int( (byte & 0x7F) << shift );
            if ((byte & 0x80) == 0)
            {
                break;
            }
            shift += 7;
        }
        
        return result;
    }
    
    public static function WriteLEB128_32(output : IDataOutput, value : Int) : Void
    {
        var more : Bool = true;
        var negative : Bool = value < 0;
        var size : Int = 32;
        while (true)
        {
            var byte : Int = value & 0x0000007F;
            value >>= 7;
            //the following is unnecessary if the implementation of >>= uses an arithmetic rather than logical shift for a signed left operand
            if (negative) {
                /* sign extend */
                value = value | -(1 << (size - 7));
            }
            /* sign bit of byte is second high order bit (0x40) */
            if ((value == 0 && (byte & 0x40) == 0) || (value == -1 && (byte & 0x40) != 0))
            {
                more = false;
            }
            else
            {
                byte = byte | 0x80;
            }
            output.writeByte(byte);
        }
    }
    
    public static function WriteLEB128_u32(output : IDataOutput, value : Int) : Void
    {
        //big values cannot be written due to infinite cycle:
        if (value > 0x7f7f7f)
        {
            value = 0;
        }
        
        do
        {
            var byte : Int = value & 0x7F;
            value >>= 7;
            if (value != 0) {
                /* more bytes to come */
                byte = byte | 0x80;
            }
            output.writeByte(byte);
        }
        while ((value != 0));
    }
    
    /** Read object from stream written using AS3's AMF.*/
    public static function ReadAMF(input : IDataInput) : Dynamic
    {
        //just skip length specifier - AS3 will read object by it's own:
        cast((input), ReadLEB128_u32);
        return input.readObject();
    }
    
    /** Write object to stream using AS3's AMF.
		\param temp Specify some ByteArray to speed up writing process (avoiding temp object creation).
		*/
    public static function WriteAMF(output : IDataOutput, object : Dynamic, temp : ByteArray = null) : Void
    {
        if (temp == null)
        {
            temp = new ByteArray();
        }
        else
        {
            temp.position = 0;
        }
        
        temp.writeObject(object);
        WriteLEB128_u32(output, temp.position);
        output.writeBytes(temp);
    }

    public function new()
    {
    }
}
)====="