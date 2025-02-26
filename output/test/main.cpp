

#include "../messenger.h"

#include "../outputs/console_output.h"


int main()
{
	Messenger messenger;

	messenger.add(new ConsoleOutput);

	messenger.write("write");
	messenger.endl();
	messenger.common("common");
	messenger.endl();
	messenger.info("info");
	messenger.endl();
	messenger.trace("trace");
	messenger.endl();
	messenger.debug("debug");
	messenger.endl();
	messenger.todo("todo");
	messenger.endl();
	messenger.warning("warning");
	messenger.endl();
	messenger.error("error");
	messenger.endl();
	messenger.fatal("fatal");
	messenger.endl();

	getchar();

	return 0;
}

