import boto3
# from boto3.dynamodb.conditions import Key
from datetime import datetime, timedelta
from dateutil.relativedelta import relativedelta


def lambda_handler(event, context):

    span = 'today'
    now = datetime.today() + timedelta(hours=9)

    if 'span' in event:
        span = event['span']

    if span == 'today':
        starttm = '{0:%Y%m%d}'.format(now) + '000000'

    elif span == '3days':
        starttm = '{0:%Y%m%d}'.format(now + timedelta(days=-3)) + '000000'

    elif span == '7days':
        starttm = '{0:%Y%m%d}'.format(now + timedelta(days=-7)) + '000000'

    elif span == '1month':
        starttm = '{0:%Y%m%d}'.format(
            now + relativedelta(months=-1)) + '000000'

    elif span == '3months':
        starttm = '{0:%Y%m%d}'.format(
            now + relativedelta(months=-3)) + '000000'

    elif span == '1year':
        starttm = '{0:%Y%m%d}'.format(now + relativedelta(years=-1)) + '000000'

    else:
        starttm = '{0:%Y%m%d}'.format(now) + '000000'

    print(starttm)

    dynamoDB = boto3.resource("dynamodb")
    table = dynamoDB.Table("moisture_data")
    queryData = table.query(
        KeyConditionExpression="dt = :dts and dt_val >= :val",
        ExpressionAttributeValues={
            ":dts": 'dev_room',
            ":val": int(starttm)
        },
        ScanIndexForward=True,  # 降順でソート
    )

    return queryData['Items']
