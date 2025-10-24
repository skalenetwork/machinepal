// @ts-check
// Note: type annotations allow type checking and IDE autocompletion

const lightCodeTheme = require('prism-react-renderer/themes/dracula');
const darkCodeTheme = require('prism-react-renderer/themes/vsDark');

/** @type {import('@docusaurus/types').Config} */
const config = {
    title: 'MachinePay Docs',
    tagline: 'Privacy, fairness, and AI on the blockchain.',
    favicon: 'img/favicon.ico',

    // GitHub Pages deployment config
    url: 'https://machinepay.github.io', // Your GitHub Pages URL
    baseUrl: '/docs/',                   // Base URL path for your site
    organizationName: 'skalenetwork',      // GitHub org/user name
    projectName: 'docs',                 // Repo name
    trailingSlash: false,

    onBrokenLinks: 'throw',
    onBrokenMarkdownLinks: 'warn',

    i18n: {
        defaultLocale: 'en',
        locales: ['en'],
    },

    presets: [
        [
            'classic',
            ({
                docs: {
                    path: 'docs',
                    routeBasePath: '/', // Serve docs at site root
                    sidebarPath: require.resolve('./sidebars.js'),
                    editUrl: 'https://github.com/machinepay/docs/edit/main/',
                    include: ['**/*.md', '**/*.mdx', 'README.md'], // include README.md
                    showLastUpdateAuthor: true,
                    showLastUpdateTime: true,
                },
                blog: false, // Disable blog
                theme: {
                    customCss: require.resolve('./src/css/custom.css'),
                },
            }),
        ],
    ],

    themeConfig: ({
        image: 'img/og-image.png',
        colorMode: {
            defaultMode: 'light',
            disableSwitch: false,
            respectPrefersColorScheme: true,
        },
        navbar: {
            title: 'MachinePay Docs',
            logo: {
                alt: 'MachinePay Logo',
                src: 'img/logo.svg',
            },
            items: [
                { to: '/', label: 'Docs', position: 'left' },
                {
                    href: 'https://github.com/machinepay',
                    label: 'GitHub',
                    position: 'right',
                },
            ],
        },
        footer: {
            style: 'dark',
            links: [
                {
                    title: 'Docs',
                    items: [
                        { label: 'Introduction', to: '/' },
                    ],
                },
                {
                    title: 'Community',
                    items: [
                        { label: 'GitHub', href: 'https://github.com/machinepay' },
                        { label: 'Twitter', href: 'https://twitter.com/machinepay' },
                    ],
                },
            ],
            copyright: `© ${new Date().getFullYear()} MachinePay. All rights reserved.`,
        },
        prism: {
            theme: lightCodeTheme,
            darkTheme: darkCodeTheme,
            additionalLanguages: ['bash', 'json', 'solidity', 'python', 'cpp'],
        },
    }),
};

module.exports = config;
