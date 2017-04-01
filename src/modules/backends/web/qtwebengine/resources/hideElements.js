var whitelist = [%1];
var blacklist = [%2];
var ignoredElements = [];

for (var i = 0; i < whitelist.length; ++i)
{
	try
	{
		ignoredElements = ignoredElements.concat(Array.from(document.querySelectorAll(whitelist[i])));
	}
	catch (e)
	{
		console.error("Invalid selector: " + whitelist[i]);

		continue;
	}
}

for (var i = 0; i < blacklist.length; ++i)
{
	var elements = [];

	try
	{
		elements = document.querySelectorAll(blacklist[i]);
	}
	catch (e)
	{
		console.error("Invalid selector: " + blacklist[i]);

		continue;
	}

	for (var j = 0; j < elements.length; ++j)
	{
		if (ignoredElements.indexOf(elements[j]) < 0)
		{
			elements[j].style.cssText = "display:none !important";
		}
	}
}
