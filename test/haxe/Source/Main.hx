package;

import openfl.display.Sprite;
import openfl.utils.Assets;

import design.Infos;

class Main extends Sprite
{
	public function new()
	{
		super();

        final data = Assets.getBytes( "assets/xls2xj.bin" );
        trace( "Data length: ", data.length );
        
        final infos = new Infos(
            data,
            ( progress : Float ) -> {
                trace( "Progress: ", progress);
            },
            () -> {
                trace( 'Done.' );
            },
        );
	}
}
