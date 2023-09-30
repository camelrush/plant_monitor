import os
import requests

# AWS lambda の環境変数に、以下を設定すること
ACCESS_TOKEN = os.environ["access_token"]   # LINEアクセストークン
CHART_URL = os.environ["chart_url"]         # グラフWebページのURL
MOIST_THRESHOLD = os.environ["threshold"]   # 水不足の閾値%(例.10)

# LINE MessagingAPI URL・ヘッダ
URL = "https://api.line.me/v2/bot/message/broadcast"
HEADERS = {"Authorization": "Bearer %s" %
           ACCESS_TOKEN, "Content-Type": "application/json"}


def lambda_handler(event, context):

    for record in event['Records']:

        # 現在の水分量を取得
        if 'NewImage' in record['dynamodb']:
            cur_moist = record['dynamodb']['NewImage']['moisture']['N']
        else:
            print('no NewImage.')
            return {
                'statusCode': 200
            }

        # 水分量が十分ならば処理終了
        if float(cur_moist) >= MOIST_THRESHOLD:
            return {
                'statusCode': 200
            }

        # 水分量が不足していれば、LINE通知メッセージを作成
        msg = '水くれ〜(T-T\nカラカラじゃよー'
        link = '<a href=\"' + CHART_URL + '\">水分グラフ</a>'
        payload = "土壌水分 {0:.1f}%じゃ。\n{1}\n{2}".format(
            float(cur_moist), msg, link)

        # 送信データ設定
        send_data = {"messages": [{"type": "text", "text": payload}]}

        # LINEに通知
        r = requests.post(URL, headers=HEADERS, json=send_data)

    return {
        'statusCode': 200,
        'body': 'HTTPStatusFromLineAPI:' + str(r.status_code)
    }
