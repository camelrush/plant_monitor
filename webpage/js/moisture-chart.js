reloadChart = (span) => {
  var url = axios
    .get(
      "( ここに API Gatewayのurlを設定します。 例.https://.....amazonaws.com/[stage] )" + 
      "?span=" + span
    )
    // thenで成功した場合の処理
    .then((response) => {
      var labels = [];
      var moists = [];
      var temps = [];
      var humis = [];

      console.log(response);
      var moistlist = response.data;
      for (var i = 0; i < moistlist.length; i++) {
        var dt_val = moistlist[i].dt_val.toString();
        var dt_raw = new Date(
          dt_val.slice(0, 4),
          dt_val.slice(4, 6),
          dt_val.slice(6, 8),
          dt_val.slice(8, 10),
          dt_val.slice(10, 12),
          dt_val.slice(12, 14)
        );
        var dt_label =
          dt_raw.getMonth().toString().padStart(2, " ") +
          "月" +
          dt_raw.getDate().toString().padStart(2, " ") +
          "日" +
          " " +
          dt_raw.getHours().toString().padStart(2, " ") +
          "時" +
          dt_raw.getMinutes().toString().padStart(2, " ") +
          "分";
        labels[i] = dt_label;
        moists[i] = parseFloat(moistlist[i].moisture);
        temps[i] = parseFloat(moistlist[i].temperature);
        humis[i] = parseFloat(moistlist[i].humidity);
      }

      var ctx = document.getElementById("myLineChart");
      var myLineChart = new Chart(ctx, {
        type: "line",
        data: {
          labels: labels,
          datasets: [
            {
              type: "line",
              label: "土壌水分",
              data: moists,
              borderColor: "rgba(0,0,255,0.7)",
              backgroundColor: "rgba(0,0,255,0.1)",
              borderWidth: 4
            },
            {
              type: "line",
              label: "温度",
              data: temps,
              borderColor: "rgba(255,0,0,0.7)",
              backgroundColor: "rgba(0,0,0,0)",
              yAxisID: "y-axis-2"
            },
            {
              type: "line",
              label: "湿度",
              data: humis,
              borderColor: "rgba(0,200,255,1)",
              backgroundColor: "rgba(0,0,0,0)",
            },
          ],
        },
        options: {
          title: {
            display: false,
            text: "土壌水分グラフ",
          },
          scales: {
            yAxes: [
              {
                id: "y-axis-1",
                position: "left",
                ticks: {
                  suggestedMax: 100,
                  suggestedMin: 0,
                  stepSize: 10,
                  callback: function (value, index, values) {
                    return value + "%";
                  },
                },
              },
              {
                id: "y-axis-2",
                position: "right",
                ticks: {
                  suggestedMax: 50,
                  suggestedMin: 0,
                  stepSize: 5,
                  callback: function (value, index, values) {
                    return value + "℃";
                  },
                },
              },
            ],
          },
          legend: {
            position: "top",
            labels: {
              fontSize: 18,
            },
          },
        },
      });
    })
    // catchでエラー時の挙動を定義
    .catch((err) => {
      console.log("err:", err);
    });
}
