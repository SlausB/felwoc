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
        infos = new Infos(
            data,
            () -> {
                trace( 'Done.', data.bytesAvailable );
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
        infos.forest.length == 3;
        infos.forest[ 1 ].color == "зелёное";
        infos.forest[ 2 ].kind == "сосна";
        infos.forest[ 0 ].strength == 0;
        infos.forest[ 2 ].mass == 14;
    }
}
