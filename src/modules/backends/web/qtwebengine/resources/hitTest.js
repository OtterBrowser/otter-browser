function normalizeUrl(url)
{
	let element = document.createElement('a');
	element.href = url;

	return element.href;
}

function getOrigin(url)
{
	let element = document.createElement('a');
	element.href = url;

	return element.origin;
}

function createHitTest(window, element, result)
{
	if (!element)
	{
		return result;
	}

	let geometry = element.getBoundingClientRect();

	result.geometry = { x: geometry.top, y: geometry.left, w: geometry.width, h: geometry.height };
	result.tagName = element.tagName.toLowerCase();

	if (result.tagName == 'frame' || result.tagName == 'iframe')
	{
		result.frameUrl = normalizeUrl(element.src);

		if (getOrigin(document.location.href) == getOrigin(element.src))
		{
			let frameDocument = element.contentWindow.document;

			result.title = frameDocument.title;
			result.offset = [(result.offset[0] + geometry.left), (result.offset[1] + geometry.top)];

			return createHitTest(element.contentWindow, ((%1 >= 0) ? frameDocument.elementFromPoint((%1 - result.offset[0]), (%2 - result.offset[1])) : frameDocument.activeElement), result);
		}

		return result;
	}

	let anchorElement = element.closest('a[href]');
	let titledElement = element.closest('[title]');

	if (anchorElement)
	{
		result.linkUrl = anchorElement.href;
	}

	if (titledElement)
	{
		result.title = titledElement.title;
	}

	if (window.getSelection().containsNode(element, true))
	{
		result.flags |= 8;
	}

	if (result.tagName == 'img')
	{
		result.alternateText = element.alt;
		result.imageUrl = normalizeUrl(element.src);
		result.longDescription = element.longDesc;
	}
	else if (result.tagName == 'audio' || result.tagName == 'video')
	{
		if (element.controls)
		{
			result.flags |= 16;
		}

		if (element.loop)
		{
			result.flags |= 32;
		}

		if (element.muted)
		{
			result.flags |= 64;
		}

		if (element.paused)
		{
			result.flags |= 128;
		}

		result.mediaUrl = normalizeUrl(element.src);
		result.playbackRate = element.playbackRate;
	}

	if (result.tagName == 'input' || result.tagName == 'textarea')
	{
		let type = (element.type ? element.type.toLowerCase() : '');

		if (result.tagName == 'textarea' || type == 'email' || type == 'password' || type == 'search' || type == 'tel' || type == 'text' || type == 'url')
		{
			if (!element.hasAttribute('disabled') && !element.hasAttribute('readonly'))
			{
				result.flags |= 1;
			}

			if (!element.value || element.value == '')
			{
				result.flags |= 2;
			}
		}
	}
	else if (element.isContentEditable)
	{
		result.flags |= 1;
	}

	if (result.tagName == 'button' || result.tagName == 'input' || result.tagName == 'select'|| result.tagName == 'textarea')
	{
		let formElement = element.closest('form');

		if (formElement)
		{
			let type = (element.type ? element.type.toLowerCase() : '');

			if ((result.tagName == 'button'|| result.tagName == 'input') && (type == 'image' || type == 'submit'))
			{
				result.formUrl = normalizeUrl(element.hasAttribute('formaction') ? element.getAttribute('formaction') : formElement.action);
			}

			if (formElement.querySelectorAll('input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]').length > 0)
			{
				result.flags |= 4;
			}
		}
	}

	return result;
}

createHitTest(window, ((%1 >= 0) ? document.elementFromPoint(%1, %2) : document.activeElement), {
	alternateText: '',
	flags: 0,
	formUrl: '',
	frameUrl: '',
	geometry: { x: -1, y: -1, w: 0, h: 0 },
	imageUrl: '',
	linkUrl: '',
	longDescription: '',
	mediaUrl: '',
	offset: [0, 0],
	position: [%1, %2],
	tagName: '',
	title: ''
});
