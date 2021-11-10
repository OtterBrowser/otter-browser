function getLinks(selector)
{
	let elements = document.querySelectorAll(selector);
	let urls = [];
	let links = [];

	for (let i = 0; i < elements.length; ++i)
	{
		if (urls.includes(elements[i].href))
		{
			continue;
		}

		urls.push(elements[i].href);

		let link = {
			title: elements[i].title.trim(),
			mimeType: elements[i].type,
			url: elements[i].href
		};

		if (link.title == '')
		{
			link.title = elements[i].textContent.trim();
		}

		if (link.title == '')
		{
			let imageElement = elements[i].querySelector('img[alt]:not([alt=\'\'])');

			if (imageElement)
			{
				link.title = imageElement.alt;
			}
		}

		links.push(link);
	}

	return links;
}

getLinks('%1');
