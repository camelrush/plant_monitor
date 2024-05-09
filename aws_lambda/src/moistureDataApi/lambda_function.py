import boto3
from datetime import datetime, timedelta
from dateutil.relativedelta import relativedelta
from collections import defaultdict
import statistics

def lambda_handler(event, context):

    span = 'today'
    now = datetime.today() + timedelta(hours=9)

    if 'span' in event:
        span = event['span']

    if span == 'today':
        starttm = '{0:%Y%m%d}'.format(now) + '000000'
        group_format = '%Y%m%d%H%M'

    elif span == '3days':
        starttm = '{0:%Y%m%d}'.format(now + timedelta(days=-3)) + '000000'
        group_format = '%Y%m%d%H'

    elif span == '7days':
        starttm = '{0:%Y%m%d}'.format(now + timedelta(days=-7)) + '000000'
        group_format = '%Y%m%d%H'

    elif span == '1month':
        starttm = '{0:%Y%m%d}'.format(
            now + relativedelta(months=-1)) + '000000'
        group_format = '%Y%m%d'

    elif span == '3months':
        starttm = '{0:%Y%m%d}'.format(
            now + relativedelta(months=-3)) + '000000'
        group_format = '%Y%m%d'

    elif span == '1year':
        starttm = '{0:%Y%m%d}'.format(now + relativedelta(years=-1)) + '000000'
        group_format = '%Y%m'

    else:
        return ''

    print(starttm)

    dynamoDB = boto3.resource("dynamodb")
    table = dynamoDB.Table("moisture_data")
    queryData = table.query(
        KeyConditionExpression="dt = :dts and dt_val >= :val",
        ExpressionAttributeValues={
            ":dts": 'dev_room',
            ":val": int(starttm)
        },
        ScanIndexForward=True
    )

    grouped_data = defaultdict(lambda: {'moisture': [], 'humidity': [], 'temperature': []})
    for item in queryData['Items']:
        group_key = datetime.strptime(str(item['dt_val']), '%Y%m%d%H%M%S').strftime(group_format)
        grouped_data[group_key]['moisture'].append(item['moisture'])
        grouped_data[group_key]['humidity'].append(item['humidity'])
        grouped_data[group_key]['temperature'].append(item['temperature'])

    # 平均値を計算し、リスト形式でデータを返す
    averaged_data = []
    for key, values in grouped_data.items():
        averaged_data.append({
            'dt_val': key,
            'moisture': round(statistics.mean(values['moisture']), 1),
            'humidity': round(statistics.mean(values['humidity']), 1),
            'temperature': round(statistics.mean(values['temperature']), 1)
        })

    return averaged_data
