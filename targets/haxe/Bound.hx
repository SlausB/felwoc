R"=====(/* This file is generated using the "xls2xj" program from XLS design file.
Bugs, issues or suggestions can be sent to SlavMFM@gmail.com */

package design;

import design.Info;


/** Relation of tables names to it's data.*/
class Bound
{
	/** Table's name without any modifications. Defined within constructor.*/
	public var tableName : String;
	/** Data objects of this table. Vector of objects inherited at least from "Info". Defined within constructor.*/
	public var objects : Array< Info >;
	/** Hash value from table's lowercase name to differentiate objects of this table from any other objects.*/
	public hash : Int;
	
	/** Both name and array at construction.*/
	public function new( tableName : String, objects : Array< Info >, hash : Int )
	{
		this.tableName = tableName;
		this.objects = objects;
		this.hash = hash;
	}
}
)====="
