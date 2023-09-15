R"=====(/* This file is generated using the "xls2xj" program from XLS design file.
Bugs issues or suggestions can be sent to SlavMFM@gmail.com
*/

package design;

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

import design.FosUtils;

{INFOS}

class Infos {
{FIELDS}

    /** Array of all arrays. Each element is of type Vector< Info > */
    public final __all = new Vector< Bound >( {BOUNDS} );

    public static var on_error : ( msg : String ) -> Void = null;

    public function new(
        data : IDataInput,
        onDone : () -> Void,
        progress : ( p : Float ) -> Void = null
    ) {
{INIT_BOUNDS}
        
        new Loader( this, data, onDone, progress );
    }
	
	/** Looks for link target.*/
	public static function FindLinkTarget( where : Vector< Info >, id : Int ) : Info {
		for ( some in where ) {
			if ( Reflect.field( some, "id" ) == id ) {
				return cast( some );
			}
		}
		return cast( null );
	}
	
	/** Obtain link at specified index of specified type.
	\param from Where to lookup for link.
	\param index Place of obtaining link from (including) zero. NULL will be returned and error printed if index is out of range.
	\param context Prefix for outputting error. Type something here to get understanding of where error occured when it will.*/
	public static function GetLink( from : Link, index : Int, context : String ) : Count {
		if ( index >= from.links.length ) {
            if ( Infos.on_error != null ) {
			    Infos.on_error( "E: " + context + ": requested link at index " + index + " but there are just " + from.links.length + " links. Returning undefined." );
            }
			return cast( null );
		}
		
		return from.links[ index ];
	}
	
	/** Use this function to restore Info when it is communicated over network.
	\param hash Hash value from table's name when looking Info is described.
	\param id Identificator within it's table.
	\return Found Info or null if nothing was found.*/
	public function FindInfo( hash : Int, id : Int ) : Null< Info >
	{
		for ( bound in this.__all ) {
			if ( hash == bound.hash ) {
				for ( info in bound.objects ) {
					if ( id == Reflect.field( info, "id" ) ) {
						return info;
					}
				}
				
				return null;
			}
		}
		return null;
	}

    /** To pretend that objects in tables of type PRECISE are plurals.*/
    private function v( table : Info ) {
        final r = new Vector< Info >( 1 );
        r[ 0 ] = table;
        return r;
    }
}

private function b( tableName : String, objects : Vector< Info >, hash : Int, precise : Bool ) {
    return new Bound( tableName, objects, hash, precise );
}

private class Loader {
    private var infos : Infos;

    /** Will be called when everything done.*/
    private var _onDone : ()->Void;
    
    /** Called to inform process completeness state from 0 to 1 if specified.*/
    private var _progress : ( p : Float )->Void;
    
    /** There are really much of such: let's cache it.*/
    private final _e : Link = new Link( new Vector< Count >( 0 ) );
	
	/** Strings cache to reuse frequently reappearing literals.*/
	private final _c = new Vector< String >( {STRINGS} );

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
    private final STRINGS_ORDER : Int = 0;
    private final OBJECTS_ORDER : Int = 1;
    private final LINKS_ORDER   : Int = 2;
    private function report_progress( order : Int, part : Float ) {
        if ( _progress == null ) {
            return;
        }

        var sum : Float = 0;
        for ( i in 0 ... load_weight.length ) {
            sum += load_weight[ i ];
        }

        var loaded : Float = 0;
        for ( i in 0 ... order ) {
            loaded += load_weight[ i ];
        }
        loaded += load_weight[ order ] * part;

        _progress( loaded / sum );
    }

    public function new(
        infos : Infos,
        data : IDataInput,
        onDone : () -> Void,
        progress : ( p : Float ) -> Void
    ) {
        this.infos = infos;
        this.data = data;
        _onDone = onDone;
        _progress = progress;

        objects_to_load = 0;
        for ( bound in infos.__all ) {
            if ( ! bound.precise ) {
                objects_to_load += bound.objects.length;
            }
        }

        strings_to_load = n();
        load_strings();
    }

    /** Read integer.*/
    private function n() {
        return FosUtils.ReadLEB128_u32( data );
    }
    /** Read real value.*/
    private function f() {
        return Std.parseFloat( s () );
    }
    /** Read string.*/
    private function s() {
        final length = n();
        return data.readMultiByte( length, "utf-8" );
    }
    /** Read array of values (integers and floats).*/
    private function a() {
        final length = n();
        final r = new Vector< Float >( length );
        for ( i in 0 ... length ) {
            r[ i ] = f();
        }
        return r;
    }
    private function r( i : Int ) {
        return infos.__all[ i ];
    }
    private function bool( v : Int ) {
        return v == 1;
    }

    /** Lets browser take a break (some systems might even kill the application if it's unresponsive for too long). */
    private function chill( cb : ()->Void ) {
        haxe.Timer.delay(
            () -> {
                cb();
            },
            1,
        );
    }

    private function load_strings() {
        for ( i in 0 ... 1000 ) {
            //all strings loaded:
            if ( strings_loaded >= strings_to_load ) {
                load_objects();
                return;
            }
            else {
                _c[ strings_loaded ] = s();
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
                while (
                    loading_type < infos.__all[ loading_type ].objects.length
                    &&
                    infos.__all[ loading_type ].precise
                ) {
                    loading_type += 1;
                }
                //all types loaded:
                if ( loading_type >= infos.__all[ loading_type ].objects.length ) {
                    links_to_load = n();
                    load_links();
                    return;
                }
                loaded_of_type = 0;
            }
            else {
                load_type( loading_type, loaded_of_type );
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
                _onDone();
                return;
            }
            final source_type = n();
            final source_id = n();
            final field_type = n();
            final count = n();
            final link = new Link( new Vector< Count >( count ) );
            for ( c in 0 ... count ) {
                final target_type = n();
                final target_id = n();
                final quantity = n();
                link.links[ c ] = new Count(
                    Infos.FindLinkTarget(
                        infos.__all[ target_type ].objects,
                        target_id
                    ),
                    quantity
                );
            }
            link_type(
                Infos.FindLinkTarget(
                    infos.__all[ source_type ].objects,
                    source_id
                ),
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