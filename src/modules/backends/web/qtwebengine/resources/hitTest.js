var element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement);
var result = {
	alternateText: '',
	flags: 0,
	formUrl: '',
	frameUrl: '',
	geometry: { x: -1, y: -1, w: 0, h: 0 },
	imageUrl: '',
	linkUrl: '',
	longDescription: '',
	mediaUrl: '',
	tagName: '',
	title: ''
};

if (element)
{
	var geometry = element.getBoundingClientRect();

	result.geometry = { x: geometry.top, y: geometry.left, w: geometry.width, h: geometry.height };

	if (element.isContentEditable)
	{
		result.flags |= 1;
	}

	if (window.getSelection().containsNode(element, true))
	{
		result.flags |= 8;
	}

	result.tagName = element.tagName.toLowerCase();

	if (result.tagName == 'iframe' || result.tagName == 'frame')
	{
		var link = document.createElement('a');
		link.href = element.src;

		result.frameUrl = link.href;
	}

	if (result.tagName == 'img')
	{
		var link = document.createElement('a');
		link.href = element.src;

		result.alternateText = element.alt;
		result.imageUrl = link.href;
		result.longDescription = element.longDesc;
	}

	if (result.tagName == 'audio' || result.tagName == 'video')
	{
		var link = document.createElement('a');
		link.href = element.src;

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

		result.mediaUrl = link.href;
	}

	if (result.tagName == 'textarea' || (result.tagName == 'input' && element.type && (element.type == 'text' || element.type == 'search')))
	{
		if (!(result.flags & 1) && !element.hasAttribute('readonly') && !element.hasAttribute('disabled'))
		{
			result.flags |= 1;
		}

		if (!element.value || element.value == '')
		{
			result.flags |= 2;
		}
	}

	var isForm = (result.tagName == 'textarea' || result.tagName == 'select' || ((result.tagName == 'input'|| result.tagName == 'button') && element.type && (element.type.toLowerCase() == 'text' || element.type.toLowerCase() == 'search')));
	var isSubmit = ((result.tagName == 'input'|| result.tagName == 'button') && element.type && (element.type.toLowerCase() == 'submit' || element.type.toLowerCase() == 'image'));

	while (element && ((isForm && !(result.flags & 4)) || (isSubmit && result.formUrl == '') || result.linkUrl == '' || result.title == ''))
	{
		if (element != document && element.title !== '')
		{
			result.title = element.title;
		}

		if (element.tagName)
		{
			if (element.tagName.toLowerCase() == 'a')
			{
				result.linkUrl = element.href;
			}
			else if ((isForm || (isSubmit && result.formUrl == '')) && element.tagName.toLowerCase() == 'form')
			{
				if ((isSubmit && result.formUrl == ''))
				{
					var link = document.createElement('a');
					link.href = element.action;

					result.formUrl = link.href;
				}
				else
				{
					result.flags |= 4;
				}
			}
		}

		element = element.parentNode;
	}
}

result;
