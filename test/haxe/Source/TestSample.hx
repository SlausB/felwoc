import utest.Assert;
import utest.Async;
import openfl.Assets;
import openfl.utils.IDataInput;

import design.Infos;

class TestSample extends utest.Test {
    var infos : Infos;
    var data : IDataInput;

    @:timeout(5000)
    public function setupClass( async : Async ) {
        data = Assets.getBytes( "assets/xls2xj.bin" );
        js.html.Console.log( "Creating infos ..." );
        infos = new Infos(
            data,
            () -> {
                trace( 'Done.', data.bytesAvailable );
                js.html.Console.log( "Infos:", infos );
                js.html.Console.log( "bytesAvailable:", data.bytesAvailable );
                async.done();
            },
            ( progress : Float ) -> {
                trace( "Progress: ", progress );
            },
        );
    }

    function specData() {
        data;
        data.bytesAvailable == 0;
    }

    function specForest() {
        /*infos.forest.length == 3;
        haxe.Utf8.validate( infos.forest[ 1 ].color );
        haxe.Utf8.validate( infos.forest[ 2 ].kind );
        infos.forest[ 1 ].color == "зелёное";
        infos.forest[ 2 ].kind == "şam ağacı";
        infos.forest[ 0 ].strength == 0;
        infos.forest[ 2 ].mass == 14;*/
    }
}
