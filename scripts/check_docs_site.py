#!/usr/bin/env python3
"""Check the published HTML contract, or collect a read-only live SEO baseline."""

import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from html.parser import HTMLParser
import json
from pathlib import Path
from urllib.parse import unquote, urljoin, urlsplit
from urllib.request import urlopen
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
BASE = 'https://jtenniswood.github.io/espcontrol/'


def normalized(value):
    return ' '.join(value.replace('\u200b', '').split())


class Page(HTMLParser):
    def __init__(self, html):
        super().__init__(convert_charrefs=True)
        self.title = ''
        self.meta = {}
        self.canonicals = []
        self.h1 = []
        self.ids = set()
        self.links = []
        self.schemas = []
        self.faq = []
        self.in_main = False
        self.capture = None
        self.schema = None
        self.question = None
        self.answer = []
        self.question_heading = False
        self.feed(html)

    def finish_answer(self):
        if self.question is not None:
            self.faq.append((normalized(self.question), normalized(''.join(self.answer))))
        self.question = None
        self.answer = []

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if attrs.get('id'):
            self.ids.add(attrs['id'])
        if tag == 'a' and attrs.get('href'):
            self.links.append(attrs['href'])
        if tag == 'meta':
            self.meta[attrs.get('name', attrs.get('property', attrs.get('http-equiv', '')))] = attrs.get('content', '')
        if tag == 'link' and attrs.get('rel') == 'canonical':
            self.canonicals.append(attrs.get('href'))
        if tag == 'main':
            self.in_main = True
        if self.in_main and tag in ('p', 'li', 'br'):
            self.answer.append(' ')
        if tag == 'title':
            self.capture = 'title'
        if tag == 'h1':
            self.h1.append('')
            self.capture = 'h1'
        if tag == 'script' and attrs.get('type') == 'application/ld+json':
            self.schema = ''
        if self.in_main and tag in ('h2', 'h3'):
            self.finish_answer()
            if tag == 'h3':
                self.question = ''
                self.question_heading = True

    def handle_endtag(self, tag):
        if tag == self.capture:
            self.capture = None
        if tag == 'h3':
            self.question_heading = False
        if tag == 'main':
            self.finish_answer()
            self.in_main = False
        if tag == 'script' and self.schema is not None:
            self.schemas.append(json.loads(self.schema))
            self.schema = None

    def handle_data(self, data):
        if self.capture == 'title':
            self.title += data
        elif self.capture == 'h1':
            self.h1[-1] += data
        if self.schema is not None:
            self.schema += data
        if self.in_main and self.question is not None:
            if self.question_heading:
                self.question += data
            else:
                self.answer.append(data)


def text_only(html):
    class Text(HTMLParser):
        def __init__(self):
            super().__init__()
            self.parts = []

        def handle_data(self, data):
            self.parts.append(data)

        def handle_starttag(self, tag, attrs):
            if tag in ('p', 'li', 'br'):
                self.parts.append(' ')

    parser = Text()
    parser.feed(html)
    return normalized(''.join(parser.parts))


def sitemap_urls(xml):
    return [node.text for node in ET.fromstring(xml).findall('.//{*}loc')]


def snapshot(pages):
    titles = Counter(page.title for page in pages.values())
    descriptions = Counter(page.meta.get('description', '') for page in pages.values())
    return {
        'captured_at': datetime.now(timezone.utc).isoformat(),
        'sitemap_urls': len(pages),
        'generated_urls': [url for url in pages if '/generated/' in url],
        'duplicate_titles': {title: count for title, count in titles.items() if count > 1},
        'duplicate_descriptions': {description: count for description, count in descriptions.items() if count > 1},
        'missing_h1': [url for url, page in pages.items() if not page.h1],
        'missing_metadata': [url for url, page in pages.items() if not page.title or not page.meta.get('description') or not page.canonicals],
        'search_console_data': 'Not available in this crawl; export from the verified owner account.',
    }


def live_snapshot(base):
    def get(url):
        with urlopen(url, timeout=30) as response:
            return response.read().decode()

    urls = sitemap_urls(get(urljoin(base, 'sitemap.xml')))
    with ThreadPoolExecutor(max_workers=6) as executor:
        return snapshot(dict(zip(urls, executor.map(lambda url: Page(get(url)), urls))))


def check_build(dist):
    errors = []
    pages = {}
    urls = sitemap_urls((dist / 'sitemap.xml').read_text())
    for url in urls:
        if not url.startswith(BASE):
            errors.append(f'Sitemap URL outside the docs site: {url}')
            continue
        relative = url[len(BASE):]
        file = dist / (relative + 'index.html' if relative.endswith('/') or not relative else relative + '.html')
        page = Page(file.read_text())
        pages[url] = page
        if '/generated/' in url or url.endswith('/card-types/weather-forecast'):
            errors.append(f'Internal or retired page in sitemap: {url}')
        if not page.title or not page.meta.get('description') or not page.meta.get('og:description'):
            errors.append(f'Missing page metadata: {url}')
        if page.canonicals != [url]:
            errors.append(f'Incorrect canonical: {url}: {page.canonicals}')
        if len(page.h1) != 1:
            errors.append(f'Expected one main heading: {url}')
        if 'noindex' in page.meta.get('robots', ''):
            errors.append(f'Noindex page in sitemap: {url}')
        if 'Espcontrol' in page.title:
            errors.append(f'Inconsistent brand spelling: {url}')

    report = snapshot(pages)
    for field in ('duplicate_titles', 'duplicate_descriptions'):
        if report[field]:
            errors.append(f'{field}: {report[field]}')

    for url, page in pages.items():
        for href in page.links:
            target = urlsplit(urljoin(url, href))
            if target.netloc != urlsplit(BASE).netloc or not target.path.startswith('/espcontrol/'):
                continue
            target_url = target._replace(query='', fragment='').geturl()
            if target_url.endswith('.html'):
                target_url = target_url[:-5]
            destination = pages.get(target_url)
            if destination:
                if target.fragment and unquote(target.fragment) not in destination.ids:
                    errors.append(f'Broken anchor from {url}: {href}')
            else:
                relative = unquote(target.path.removeprefix('/espcontrol/'))
                if not (dist / relative).is_file():
                    errors.append(f'Broken or retired link from {url}: {href}')

    faq = pages[BASE + 'reference/faq']
    schemas = [schema for schema in faq.schemas if schema.get('@type') == 'FAQPage']
    if len(schemas) != 1:
        errors.append('FAQ must have exactly one generated FAQPage schema')
    else:
        actual = [(entry['name'], text_only(entry['acceptedAnswer']['text'])) for entry in schemas[0]['mainEntity']]
        if actual != faq.faq:
            errors.append('FAQ structured answers differ from the rendered questions or answers')

    redirects = {
        'reference/request-device-support': BASE + 'screens/',
        'card-types/weather-forecast': BASE + 'card-types/weather#temperatures-tomorrow',
        'generated/cards/capabilities': BASE + 'reference/card-capabilities',
        'generated/cards/runtime-coverage': 'https://github.com/jtenniswood/espcontrol/blob/main/dev-docs/generated/card-runtime-coverage.md',
    }
    screen_guides = {
        '4848s040': ('4848s040#card-grid', '4848s040'),
        'jc1060p470': ('jc1060p470#card-grid', 'jc1060p470-v1'),
        'jc1060p470-v2': ('jc1060p470#card-grid', 'jc1060p470-v2'),
        'jc4880p443': ('jc4880p443#card-grid', 'jc4880p443'),
        'jc8012p4a1': ('jc8012p4a1', 'jc8012p4a1-v1'),
        'jc8012p4a1-v2': ('jc8012p4a1', 'jc8012p4a1-v2'),
        'jc8012p4a1-v3': ('jc8012p4a1', 'jc8012p4a1-v3'),
        'p4-86': ('p4-86#card-grid', 'p4-86'),
    }
    for model, (grid, install) in screen_guides.items():
        redirects[f'generated/screens/{model}-grid'] = BASE + 'screens/' + grid
        redirects[f'generated/screens/{model}-install'] = BASE + 'screens/' + install + '#install'
    baseline = json.loads((ROOT / 'dev-docs/hosting/seo-baseline-2026-09-22.json').read_text())
    for url in baseline['generated_urls']:
        if url.removeprefix(BASE) not in redirects:
            errors.append(f'Previously published generated page has no redirect: {url}')
    for path, destination in redirects.items():
        file = dist / (path + '.html')
        if not file.is_file():
            errors.append(f'Missing retired-page redirect: {path}')
            continue
        page = Page(file.read_text())
        if page.canonicals != [destination.split('#')[0]] or 'noindex' not in page.meta.get('robots', '') or page.meta.get('refresh') != f'0; url={destination}' or destination not in page.links:
            errors.append(f'Broken retired-page redirect: {path}')
        if destination.startswith(BASE):
            target = urlsplit(destination)
            target_page = pages.get(target._replace(fragment='').geturl())
            if not target_page or (target.fragment and unquote(target.fragment) not in target_page.ids):
                errors.append(f'Broken redirect destination: {path}: {destination}')
    for file in (dist / 'generated').rglob('*.html'):
        if file.relative_to(dist).with_suffix('').as_posix() not in redirects:
            errors.append(f'Generated fragment is still published: {file.relative_to(dist)}')
    if (dist / 'robots.txt').exists():
        errors.append('Project-path robots.txt cannot control crawling; publish the host-root template instead')

    if errors:
        raise SystemExit('\n'.join(sorted(set(errors))))
    report['faq_answers_verified'] = len(faq.faq)
    print(f'Docs site checks passed: {len(pages)} canonical pages, {len(faq.faq)} matching FAQ answers, {len(redirects)} redirects; internal links and anchors checked.')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dist', type=Path, default=ROOT / 'docs/.vitepress/dist')
    parser.add_argument('--base-url', help='Read-only live sitemap crawl instead of checking the local build')
    parser.add_argument('--output', type=Path, help='Write the measured technical baseline as JSON')
    args = parser.parse_args()
    result = live_snapshot(args.base_url) if args.base_url else check_build(args.dist)
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    elif args.base_url:
        print(json.dumps(result, indent=2))
