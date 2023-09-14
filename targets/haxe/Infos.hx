R"=====(/* This file is generated using the "xls2xj" program from XLS design file.
Bugs issues or suggestions can be sent to SlavMFM@gmail.com
*/

package design;

import com.junkbyte.console.Cc;
import haxe.Timer;
import openfl.net.URLLoader;
import openfl.net.URLRequest;
import openfl.net.URLLoaderDataFormat;
import openfl.events.ProgressEvent;
import openfl.events.Event;
import openfl.utils.IDataInput;
import openfl.Vector;

import design.Link;
import design.Info;
import design.Bound;

import fos.FosUtils;

{INFOS}

class Infos {
{FIELDS}

    /** Array of all arrays. Each element is of type Vector< Info > */
    public final __all = new Vector< Bound >( {BOUNDS} );

    public function new(
        data : IDataInput,
        onDone : Function,
        progress : Function
    ) {
{INIT_BOUNDS}
        
        new Loader( data, onDone, progress );
    }
	
	/** Looks for link target.*/
	public static function FindLinkTarget( where : Array< Info >, id : Int ) : Info {
		for ( i => some in where ) {
			if ( some.id == id ) {
				return cast( some );
			}
		}
		return cast( null );
	}
	
	/** Obtain link at specified index of specified type.
	\param from Where to lookup for link.
	\param index Place of obtaining link from (including) zero. NULL will be returned and error printed if index is out of range.
	\param context Prefix for outputting error. Type something here to get understanding of where error occured when it will.*/
	public static GetLink( from : Link, index : Int, context : String ) : Count {
		if ( index >= from.links.length ) {
			Cc.error( "E: " + context + ": requested link at index " + index + " but there are just " + from.links.length + " links. Returning undefined." );
			return cast( null );
		}
		
		return from.links[ index ];
	}
	
	/** Use this function to restore Info when it is communicated over network.
	\param hash Hash value from table's name when looking Info is described.
	\param id Identificator within it's table.
	\return Found Info or null if nothing was found.*/
	public FindInfo( hash : Int, id : Int ) : Null< Info >
	{
		for ( i => bound in this.__all ) {
			if ( hash == bound.hash ) {
				for ( oi => info in bound.objects ) {
					if ( id == info.id ) {
						return info;
					}
				}
				
				return null;
			}
		}
		return null;
	}
}

private function b( tableName : String, count : Int, hash : Int ) {
    return new Bound( tableName, new Vector< Info >( count ), hash );
}

private Loader {
    private var infos : Infos;

    /** Will be called when everything done.*/
    private var _onDone : Function;
    
    /** Called to inform process completeness state from 0 to 1 if specified.*/
    private var _progress : Function;
    
    /** There are really much of such: let's cache it.*/
    private final _emptyLink : Link = new Link( new Vector< Count >( 0 ) );
	
	/** Strings cache to reuse frequently reappearing literals.*/
	private final _c = new Vector< string >( {STRINGS} )

    private var data : IDataInput;

    private var strings_to_load : Int;
    private var strings_loaded : Int = 0;

    private var objects_to_load : Int;
    private var objects_loaded : Int = 0;

    private var links_to_load : Int;
    private var links_loaded : Int = 0;

    /** Currently loading type id.*/
    private var loading_type : Int = 0;
    /** How much objects of loading_type already loaded.*/
    private var loaded_of_type : Int = -1;

    /** How much every specific part weights in terms of loading burden.*/
    private final load_weight = [ 2, 4, 2 ];
    private final STRINGS_ORDER : Float = 1;
    private final OBJECTS_ORDER : Float = 2;
    private final LINKS_ORDER   : Float = 3;
    private function report_progress( order : Int, part : Float ) {
        if ( ! _progress ) {
            return;
        }

        var sum : Number = 0;
        for ( i in 0 ... load_weight.length ) {
            sum += load_weight[ i ];
        }

        var loaded : Number = 0;
        for ( i in 0 ... order ) {
            loaded += load_weight[ i ];
        }
        loaded += load_weight[ order ] * part;

        _progress( loaded / sum );
    }

    public function new(
        infos : Infos,
        data : IDataInput,
        onDone : Function,
        progress : Function
    ) {
        this.infos = infos;
        this.data = data;
        _onDone = onDone;
        _progress = progress;

        strings_to_load = FosUtils.ReadLEB128_u32( data );
        load_strings();
    }

    /** Lets browser to take a break (some systems might even kill the application if it's unresponsive for too long). */
    private function chill( cb : Function ) {
        haxe.Timer.delay(
            () => {
                cb();
            },
            1,
        )
    }

    private function load_strings() {
        for ( i in 0 ... 1000 ) {
            //all strings loaded:
            if ( strings_loaded >= strings_to_load ) {
                objects_to_load = FosUtils.ReadLEB128_u32( data );
                load_objects();
                return;
            }
            else {
                final length = FosUtils.ReadLEB128_u32( data );
                _c[ strings_loaded ] = data.readMultiByte( length, "utf-8" );
                strings_loaded += 1;
            }
        }

        report_progress( STRINGS_ORDER, strings_loaded / strings_to_load );
        chill( load_strings );
    }

    private function load_objects() {
        for ( i in 0 ... 1000 ) {
            //all objects of current type successfully loaded:
            if ( loaded_of_type >= infos.__all[ loading_type ].objects.length ) {
                loading_type += 1;
                //all types loaded:
                if ( loading_type >= counts.length ) {
                    links_to_load = FosUtils.ReadLEB128_u32( data );
                    load_links();
                    return;
                }
                loaded_of_type = 0;
            }
            else {
                load_type( FosUtils.ReadLEB128_u32( data ) );
                loaded_of_type += 1;
                objects_loaded += 1;
            }
        }

        report_progress( OBJECTS_ORDER, objects_loaded / objects_to_load );
        chill( load_objects );
    }
    private function load_type( type : Int, index : Int ) {
{LOAD_TYPES}
    }

    private function load_links() {
        for ( i in 0 ... 300 ) {
            if ( links_loaded >= links_to_load ) {
                onLoad();
                return;
            }
            final source_type = FosUtils.ReadLEB128_u32( data );
            final source_id = FosUtils.ReadLEB128_u32( data );
            final field_type = FosUtils.ReadLEB128_u32( data );
            final count = FosUtils.ReadLEB128_u32( data );
            final link = new Link( new Vector< Count >( count ) );
            for ( c in 0 ... count ) {
                final target_type = FosUtils.ReadLEB128_u32( data );
                final target_id = FosUtils.ReadLEB128_u32( data );
                link.links[ c ] = infos.FindLinkTarget( infos.__all[ target_type ], target_id );
            }
            link_type(
                infos.FindLinkTarget( infos.__all[ source_type ], source_id ),
                source_type,
                field_type,
                link
            );

            links_loaded += 1;
        }

        report_progress( LINKS_ORDER, links_loaded / links_to_load );
        chill( load_links );
    }
    private function link_type( o : Info, type : Int, field : Int, l : Link ) {
{LINK_TYPES}
    }
}

)====="