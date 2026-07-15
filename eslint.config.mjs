// ESLint for the browser simulator (sim/). Quality via eslint:recommended,
// consistency via a small @stylistic set matching the repo's Google-ish C++
// style (2-space indent, single quotes, 80 columns, semicolons).
import js from '@eslint/js';
import stylistic from '@stylistic/eslint-plugin';

export default [
  js.configs.recommended,
  {
    files: ['sim/**/*.js', 'sim/**/*.mjs'],
    plugins: { '@stylistic': stylistic },
    languageOptions: {
      ecmaVersion: 2022,
      sourceType: 'module',
      globals: {
        // Browser (ui.js, api.js, test.html modules)
        window: 'readonly',
        document: 'readonly',
        location: 'readonly',
        fetch: 'readonly',
        performance: 'readonly',
        requestAnimationFrame: 'readonly',
        URLSearchParams: 'readonly',
        URL: 'readonly',
        console: 'readonly',
        // Node (harness.js, vectors.test.mjs feature-detect before use)
        process: 'readonly',
      },
    },
    rules: {
      '@stylistic/indent': ['error', 2],
      '@stylistic/quotes': ['error', 'single', { avoidEscape: true }],
      '@stylistic/semi': ['error', 'always'],
      '@stylistic/max-len': ['error', { code: 80, ignoreUrls: true }],
      '@stylistic/comma-dangle': ['error', 'always-multiline'],
      'no-unused-vars': ['error', { argsIgnorePattern: '^_' }],
      eqeqeq: ['error', 'always'],
      'no-var': 'error',
      'prefer-const': 'error',
    },
  },
];
