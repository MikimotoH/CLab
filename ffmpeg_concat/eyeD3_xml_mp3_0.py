dom1 = xml.dom.minidom.parse('8minsreading_2010.xml')
items = dom1.getElementsByTagName('item')
for item in items:
    title = item.getElementsByTagName('title')[0].\
            firstChild.nodeValue
    link = item.getElementsByTagName('link')[0].\
            firstChild.nodeValue
    fileName = link.split('/')[-1].split('.')[0]
    description = item.getElementsByTagName('description')\
            [0].firstChild.nodeValue

