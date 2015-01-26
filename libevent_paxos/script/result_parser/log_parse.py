#! /usr/bin/env python2.7
import argparse
import xlwt

base_value = 4294967297

def pass_proxy(data_list,options):
    with open(options.proxy_log,"r") as proxy_input:
        count = 0
        for line in proxy_input.readlines():
            temp = line.strip().split(":")
            if count%3==0:
                temp_list = [float(x) for x in temp[3].split(",")]
                data_list.append(temp_list[:])
            count = count+1
    return data_list

def pass_con(data_list,options):
    with open(options.consensus_log,"r") as consensus_input:
        count = 0
        for line in consensus_input.readlines():
            temp = line.strip().split(":")
            data_list[count].append(float(temp[0]))
            count = count + 1
    return data_list

def write_excel(data_list):
    workbook = xlwt.Workbook()
    sheet = workbook.add_sheet("Data")
    for idx,val in enumerate(data_list):
        sheet.write(idx,0,idx+1)
        sheet.write(idx,1,val[0])
        sheet.write(idx,2,val[4])
        sheet.write(idx,3,val[3])
        sheet.write(idx,4,(val[3]-val[0])*1000)
        sheet.write(idx,5,(val[3]-val[4])*1000)
        sheet.write(idx,6,(val[4]-val[0])*1000)
    workbook.save("foobar.xls")
    pass

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-z","--zero",action="store_true")
    parser.add_argument("-p","--proxy",dest="proxy_log",type=str,required=True);
    parser.add_argument("-c","--consensus",dest="consensus_log",type=str,required=True);
    options = parser.parse_args()
    data_list = list()
    pass_proxy(data_list,options)
    pass_con(data_list,options)
    write_excel(data_list)

if __name__=="__main__":
    main()

