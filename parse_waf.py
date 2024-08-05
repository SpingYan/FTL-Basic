import csv
import pandas as pd
import matplotlib.pyplot as plt

def plot_waf_trend(filenames, labels):
    # 設定圖表大小
    plt.figure(figsize=(10, 6))

    # 使用預設的色彩列表，確保不同的檔案有不同的顏色
    colors = plt.cm.get_cmap('tab10', len(filenames))

    # 讀取每個檔案並繪製
    for i, (filename, label) in enumerate(zip(filenames, labels)):
        # 讀取 CSV 檔案
        df = pd.read_csv(filename)
        # 繪製圖表
        plt.plot(df['Loop'], df['WAF'], marker='o', label=label, color=colors(i))

    # 設定圖表標題和軸標籤
    plt.title('WAF Trend over Loop Counts')
    plt.xlabel('Loop')
    plt.ylabel('WAF')

    # 顯示圖例
    plt.legend()

    # 顯示圖表
    plt.grid(True)
    plt.show()


filenames = [
    './Bin/WAF_RND_OP_07.csv', 
    './Bin/WAF_SEQ_OP_07.csv', 
    './Bin/WAF_POS_OP_07.csv', 
    './Bin/WAF_RND_OP_14.csv', 
    './Bin/WAF_SEQ_OP_14.csv', 
    './Bin/WAF_POS_OP_14.csv', 
    './Bin/WAF_RND_OP_28.csv', 
    './Bin/WAF_SEQ_OP_28.csv', 
    './Bin/WAF_POS_OP_28.csv', 
    './Bin/WAF_RND_OP_35.csv', 
    './Bin/WAF_SEQ_OP_35.csv', 
    './Bin/WAF_POS_OP_35.csv', 
    # 可以繼續添加更多檔案路徑
]

labels = [
    'RND_OP_7%',
    'SEQ_OP_7%',
    'POS_OP_7%',
    'RND_OP_14%',
    'SEQ_OP_14%',
    'POS_OP_14%',
    'RND_OP_28%',
    'SEQ_OP_28%',
    'POS_OP_28%',
    'RND_OP_35%',
    'SEQ_OP_35%',
    'POS_OP_35%',
    # 對應的標籤
]

plot_waf_trend(filenames, labels)