import json

import scrapy


class PortalSpider(scrapy.Spider):
    name = "portal"
    allowed_domains = ["portaldeangola.com"]
    start_urls = ["https://www.portaldeangola.com/category/cultura/musica/"]
    sum = 1 + 1.4
    def parse(self, response):
        articles = response.css('.tdi_143 .td-module-container')
        data = []
        for article in articles:
            art = article.css('.td-module-thumb a')
            res = {
                'content': art.attrib['title'] + '<br>Saiba mais: ' + art.attrib['href'],
                'attachment': art.css('span').attrib['data-img-url']
            }
            data.append(res)
            yield res

            # Save dictionary to a JSON file
            filename = 'data.json'
            with open(filename, 'w') as f:
                json.dump(data, f)
