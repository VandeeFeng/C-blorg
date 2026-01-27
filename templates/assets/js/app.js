document.addEventListener('DOMContentLoaded', function () {
  // --- Fancybox Setup ---
  if (typeof Fancybox !== 'undefined') {
    document.querySelectorAll('.img').forEach(function (imgElement) {
      imgElement.setAttribute('data-fancybox', 'gallery');
    });

    Fancybox.bind("[data-fancybox='gallery']", {
      loop: true,
      buttons: ["zoom", "close"],
    });
  }

  // --- Sticky Headers with Anchors ---
  const tocLinks = document.querySelectorAll('#text-table-of-contents a');
  const tocIds = Array.from(tocLinks).map(link => {
    const href = link.getAttribute('href');
    return href ? href.substring(1) : null;
  }).filter(id => id);

  const headers = document.querySelectorAll('#content h2:not(.post-title):not(.tags-title), #content h3, #content h4');

  headers.forEach(header => {
    const headerId = header.id;
    if (!headerId) {
      console.log('Header without ID:', header.textContent);
      return;
    }

    if (tocIds.includes(headerId)) {
      const computedStyle = window.getComputedStyle(header);
      const marginTop = computedStyle.marginTop;
      const marginBottom = computedStyle.marginBottom;

      const wrapper = document.createElement('div');
      wrapper.className = 'heading-wrapper';
      wrapper.style.marginTop = marginTop;
      wrapper.style.marginBottom = marginBottom;
      wrapper.style.background = '#fdf6e3';
      wrapper.style.position = 'sticky';
      wrapper.style.top = '0';
      wrapper.style.zIndex = '100';

      const anchor = document.createElement('a');
      anchor.className = 'heading-anchor';
      anchor.href = `#${headerId}`;
      anchor.textContent = '#';
      anchor.setAttribute('aria-label', `Link to ${header.textContent}`);

      header.parentNode.insertBefore(wrapper, header);
      wrapper.appendChild(header);
      wrapper.appendChild(anchor);

      header.style.margin = '0';
      header.style.padding = '0';

      anchor.addEventListener('click', function (e) {
        e.preventDefault();
        window.location.hash = headerId;
      });
    }
  });
});
