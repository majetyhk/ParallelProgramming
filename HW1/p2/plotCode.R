
setwd("C:\\Users\\Hari Krishna M\\Dropbox\\NCSU\\ParallelSystems\\Codes\\ParallelSystems\\HW1\\p2")
library(ggplot2)
inp = read.csv("res1.csv",header = TRUE)
colnames(inp)<-c("Pair","Size","AvgRTT","StdDev")
inp$Pair = factor(inp$Pair)
inp$Size = factor(inp$Size)
plots<- ggplot(inp,aes(x = Size,y = AvgRTT,fill = Pair) ) + geom_bar(position = "dodge", stat = "identity") + geom_errorbar(aes(ymin = AvgRTT-StdDev, ymax = AvgRTT+StdDev ),width =0.8, position = "dodge") +geom_line(position = "dodge" ) + labs(y = "Round trip time(milliseconds)", x = "Message size(bytes)")
plots