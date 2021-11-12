function hideElements(allowedSelectors, disallowedSelectors)
{
	let ignoredElements = [];

	for (let i = 0; i < allowedSelectors.length; ++i)
	{
		try
		{
			ignoredElements = ignoredElements.concat(Array.from(document.querySelectorAll(allowedSelectors[i])));
		}
		catch (error)
		{
			console.error('Invalid selector: ' + allowedSelectors[i]);

			continue;
		}
	}

	for (let i = 0; i < disallowedSelectors.length; ++i)
	{
		let elements = [];

		try
		{
			elements = document.querySelectorAll(disallowedSelectors[i]);
		}
		catch (error)
		{
			console.error('Invalid selector: ' + disallowedSelectors[i]);

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
}

hideElements([%1], [%2]);
