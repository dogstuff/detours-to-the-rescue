(function () {
  function classifyPcdogsCodeTokens(root) {
    const scope = root || document;
    scope.querySelectorAll('.md-typeset .highlight code .n').forEach((token) => {
      const text = token.textContent || '';
      token.classList.remove('pcdogs-code-type', 'pcdogs-code-function', 'pcdogs-code-data');
      if (text.startsWith('DTTR_PCDOGS_T_')) {
        token.classList.add('pcdogs-code-type');
      } else if (text.startsWith('DTTR_PCDOGS_F_')) {
        token.classList.add('pcdogs-code-function');
      } else if (text.startsWith('DTTR_PCDOGS_D_')) {
        token.classList.add('pcdogs-code-data');
      }
    });
  }

  if (typeof document$ !== 'undefined') {
    document$.subscribe(() => classifyPcdogsCodeTokens(document));
  } else if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => classifyPcdogsCodeTokens(document));
  } else {
    classifyPcdogsCodeTokens(document);
  }
})();
