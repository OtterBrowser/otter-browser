(function(window)
{
	var image = document.querySelector('img');
	var drag = null;
	var dragEnd = null;
	var dragInterval = 50;
	var ignore = false;

	if (image && !image.classList.contains('imageViewer'))
	{
		var styleSheet = document.createElement('style');

		document.head.appendChild(styleSheet);

		styleSheet.sheet.insertRule('html, body {width:100%;height:100%;}', 0);
		styleSheet.sheet.insertRule('body {display:-webkit-flex;margin:0;padding:0;-webkit-align-items:center;text-align:center;}', 0);
		styleSheet.sheet.insertRule('img {max-width:100%;max-height:100%;margin:auto;-webkit-user-select:none;}', 0);
		styleSheet.sheet.insertRule('.zoomedIn {display:table;}', 0);
		styleSheet.sheet.insertRule('.zoomedIn body {display:table-cell;vertical-align:middle;}', 0);
		styleSheet.sheet.insertRule('.zoomedIn img {max-width:none;max-height:none;cursor:-webkit-zoom-out;}', 0);
		styleSheet.sheet.insertRule('.zoomedIn .drag {cursor:move;}', 0);
		styleSheet.sheet.insertRule('.zoomedOut img {cursor:-webkit-zoom-in;}', 0);

		image.classList.add('imageViewer');

		image.addEventListener('click', function(event)
		{
			var clickStart = new Date().getTime();

			if (dragEnd !== null && ((clickStart - dragEnd) < dragInterval))
			{
				return;
			}

			var documentSize = [window.innerWidth, window.innerHeight];
			var imageSize = [image.naturalWidth, image.naturalHeight];

			if (imageSize[0] <= documentSize[0] && imageSize[1] <= documentSize[1])
			{
				document.documentElement.classList.remove('zoomedIn');
				document.documentElement.classList.remove('zoomedOut');
			}
			else if (ignore && (imageSize[0] > documentSize[0] || imageSize[1] > documentSize[1]))
			{
				document.documentElement.classList.add('zoomedOut');
			}
			else if (!ignore)
			{
				if (document.documentElement.classList.contains('zoomedOut') || (image.classList.contains('loaded') && document.documentElement.classList.length === 0))
				{
					var imageComputedSize = [image.clientWidth, image.clientHeight];
					var ratioX = (imageComputedSize[0] / imageSize[0]);
					var ratioY = (imageComputedSize[1] / imageSize[1]);
					var scrollToX = (event.offsetX / ratioX) - (imageComputedSize[0] / 2);
					var scrollToY = (event.offsetY / ratioY) - (imageComputedSize[1] / 2);

					document.documentElement.classList.add('zoomedIn');
					document.documentElement.classList.remove('zoomedOut');

					window.scrollTo(scrollToX, scrollToY);
				}
				else
				{
					document.documentElement.classList.add('zoomedOut');
					document.documentElement.classList.remove('zoomedIn');
				}
			}

			ignore = false;
		});

		this.addEventListener('resize', function()
		{
			ignore = true;

			image.click();
		});

		this.addEventListener('mousedown', function(event)
		{
			if (document.documentElement.classList.contains('zoomedIn') && event.button === 0)
			{
				drag = {
					oldX: (event.screenX + this.scrollX),
					oldY: (event.screenY + this.scrollY)
				};
			}
		});

		this.addEventListener('dragstart', function(event)
		{
			if (document.documentElement.classList.contains('zoomedIn'))
			{
				event.preventDefault();
			}
		});

		this.addEventListener('mousemove', function(event)
		{
			if (drag === null)
			{
				return;
			}

			image.classList.add('drag');

			this.scrollTo((drag.oldX - event.screenX), (drag.oldY - event.screenY));
		});

		this.addEventListener('mouseup', function(event)
		{
			if (image.classList.contains('drag'))
			{
				image.classList.remove('drag');

				dragEnd = new Date().getTime();
			}

			drag = null;
		});

		if (image.complete)
		{
			image.click();
			image.classList.add('loaded');
		}
		else
		{
			image.addEventListener('load', function()
			{
				image.click();
				image.classList.add('loaded');
			});
		}

		image.removeAttribute('width');
		image.removeAttribute('height');
		image.removeAttribute('style');

		var observer = new MutationObserver(function()
		{
			image.removeAttribute('width');
			image.removeAttribute('height');
			image.removeAttribute('style');
		});
		observer.observe(image, { attributes: true });
	}
})(window);
