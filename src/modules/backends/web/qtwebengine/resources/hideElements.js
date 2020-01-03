let whitelist = [%1];
let blacklist = [%2];
let ignoredElements = [];

for (let i = 0; i < whitelist.length; ++i)
{
	try
	{
		ignoredElements = ignoredElements.concat(Array.from(document.querySelectorAll(whitelist[i])));
	}
	catch (e)
	{
		console.error('Invalid selector: ' + whitelist[i]);

		continue;
	}
}

for (let i = 0; i < blacklist.length; ++i)
{
	let elements = [];

	try
	{
		elements = document.querySelectorAll(blacklist[i]);
	}
	catch (e)
	{
		console.error('Invalid selector: ' + blacklist[i]);

		continue;
	}

	for (let j = 0; j < elements.length; ++j)
	{
		if (ignoredElements.indexOf(elements[j]) < 0)
		{
			elements[j].style.cssText = 'display:none !important';
		}
	}
}
