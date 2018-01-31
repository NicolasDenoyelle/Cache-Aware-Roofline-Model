#!/usr/bin/Rscript --vanilla --slave
#################################################################################################################################
                                        # This is a script to plot results output by main benchmark.                                                                    #
                                        #                                                                                                                               #
                                        # Author: Nicolas Denoyelle (nicolas.denoyelle@inria.fr)                                                                        #
                                        # Date: 12/11/2015 (FR format)                                                                                                  #
                                        # Version: 0.1                                                                                                                  #
#################################################################################################################################

options("width"=system(command = "tput cols", intern = T))

#################################################################################################################################
## Parse options
#################################################################################################################################

library("optparse")

##Parse options
inOpt = make_option(opt_str = c("-i", "--input"), type = "character",  default = NULL, 
                    help = "Input plateform evaluation obtained with roofline binary.")

outOpt = make_option(opt_str = c("-o", "--output"), type = "character", default = "roofline_chart.pdf", 
                     help = "Output pdf chart.")

datOpt = make_option(opt_str = c("-d", "--data"), type = "character", default = NULL, 
                     help = "Use an application trace obtained with librfsampling to plot into the roofline chart.
                Each pair(type,info) is plot as a single point even if it appears several times. 
                In the latter case, it is possible to display the deviation with -s option.
                Points are represented as a transparent circle which radius is set according to the point median runtime")

dfilterOpt = make_option(opt_str = c("-f", "--filter"), type = "character", default = NULL, 
                         help = "Filter the trace obtained with librfsampling on column info.
                Option argument is a comma separated list of items.
                For instance: ddot,scale")

validOpt = make_option(opt_str = c("-v", "--validation"), type = "logical", default=FALSE, action="store_true",
                       help = "Plot validation points acquired in the input file on the chart.")

titleOpt = make_option(opt_str = c("-t", "--title"), type = "character", default = NULL,
                       help = "Plot title")

locationOpt = make_option(opt_str = c("-l", "--location"), type = "character", default = NULL,
                          help = "Select a specific location, contained in the input. (Filter on Obj column)
                For instance: \"NUMANODE:0,NUMANODE:1\"")

bandwidthOpt = make_option(opt_str = c("-b", "--bandwidth"), type = "character", default = NULL,
                           help = "Refine plot to keep only certain bandwidths. The option argument is a comma seperated list items.
                An item as the following syntax: 'memory|type'.
                For instance: L1d:0|2LD1ST,NUMANode:0|LOAD
                Note that items in data opt will not be filtered")

legendOpt = make_option(opt_str = c("--legend"), type = "character", default="bottomright",
                        help = "Move legends on the plot. Allowed values: none, topright, topleft, bottomright, bottomleft.")

labelsOpt = make_option(opt_str = c("--labels"), type = "character", default="right",
                        help = "Move bandwidths labels on the bandwidth roofs. Allowed values: none, right, left.")

fpeakOpt = make_option(opt_str = c("-p", "--fpeak"), type = "character", default = NULL,
                       help = "Refine plot to keep only certain fpeak roofs. The option argument is a comma seperated list of the roofs.
                For instance: ADD,MUL")

groupOpt = make_option(opt_str = c("-g", "--group"), type = "character", default=NULL, action="store_true",
                      help = "Try to group output application points on the graph.
Points groups are made such the names match until the last \"_\". Then only the suffix after \"_\" is print as a label and
the prefix is print on top axis.")

statOpt = make_option(opt_str = c("-s", "--stats"), type = "logical", default=FALSE, action="store_true",
                      help = "Output deviation on bandwidths, validation and additional points into the plot.")

subtitleOpt = make_option(opt_str = c("--nosubtitle"), type = "logical", default=FALSE, action="store_true",
                      help = "Hide location subtitle on plot.")

zoomOpt = make_option(opt_str = c("-z", "--zoom"), type = "character", default=".00390625,64",
                      help = "Zoom in or out between given arithmetic intensities: %f,%f")

yminOpt = make_option(opt_str = c("--ymin"), type = "numeric", default=".1",
                      help = "Set the minimum value for y axis")

ymaxOpt = make_option(opt_str = c("--ymax"), type = "numeric", default="0",
                      help = "Set the maximum value for y axis")

aspectRatioOpt = make_option(opt_str = c("--ar"), type = "numeric", default="2",
                      help = "Change the pdf output aspect ratio.")

cexOpt = make_option(opt_str = c("--cex"), type = "numeric", default="1",
                      help = "Multiplier of font size.")

colOpt = make_option(opt_str = c("--color"), type = "integer", default="1",
                      help = "Change colors on plot. Set with an integer value > 0.")



args = commandArgs(trailingOnly = TRUE)
optParse = OptionParser(option_list = c(inOpt,
                                        outOpt,
                                        datOpt,
                                        validOpt,
                                        titleOpt,
                                        locationOpt,
                                        bandwidthOpt,
                                        fpeakOpt,
                                        statOpt,
                                        dfilterOpt,
                                        zoomOpt,
                                        yminOpt,
                                        ymaxOpt,                                                                                
                                        aspectRatioOpt,
                                        legendOpt,
                                        labelsOpt,
                                        groupOpt,
                                        cexOpt,
                                        colOpt,
                                        subtitleOpt))

options = parse_args(optParse, args=args)
srt = 45 # bandwidth angle

## options$input = "~/Documents/LARM/output/Xeon_E5-2650L_v4/joe0.roofs"
## options$data = "~/Documents/LARM/output/Xeon_E5-2650L_v4/kernels.roofs"
## options$validation = T
## options$stats = T
## options$bandwidth="NUMANode:0|2LD1ST,NUMANode:0|LOAD,NUMANode:1|2LD1ST,NUMANode:1|LOAD"
## options$filter="load|triad,load|ddot"

#################################################################################################################################
## Process options
#################################################################################################################################
if(is.null(options$input)){stop("Input option is mandatory.")}

df = read.table(options$input,header=T)
df$type=toupper(df$type)
df$location = toupper(df$location)
if(is.null(options$location)){
    locations = unique(df$location)
} else {
    locations = unlist(strsplit(toupper(options$location), split=","))
}
df$memory=toupper(df$memory)

pt = NULL
if(!is.null(options$data)){
    pt = read.table(options$data, header=T)
    pt$Location=toupper(pt$Location)
    pt$info=toupper(pt$info)
    pt$type=toupper(pt$type)
    if(!is.null(options$filter)){
        filters = unlist(strsplit(toupper(options$filter), split=","))
        new_pt = pt[0,]
        for(f in filters){
            new_pt = rbind(new_pt, pt[grepl(f, pt$info, ignore.case=T),])
        }
        pt=new_pt
    }
}


fpeak_types=NULL
if(!is.null(options$fpeak)){fpeak_types = unlist(strsplit(toupper(options$fpeak), split=","))}

bandwidth_types = NULL
if(!is.null(options$bandwidth)){
    bandwidth_types = list()
    bs = unlist(strsplit(toupper(options$bandwidth), split=","))
    for(b in bs){bandwidth_types = c(bandwidth_types, strsplit(b, split="|", fixed=T))}
}

options$labels = tolower(options$labels)
options$legend = tolower(options$legend)

display_legend <- function(){ options$legend != "none" }
display_labels <- function(){ options$labels != "none" }

#################################################################################################################################
## Extract synthetic data
#################################################################################################################################

fpeak_roofs <- function(df, types=NULL, verbose=T){
    if(verbose){print("Performance peaks:")}
    fpeaks = data.frame(type=character(0), GFlop.s=numeric(0), sd=numeric(0), nthreads=integer(0), stringsAsFactors=FALSE)
    samples = df[is.na(df$memory),]
    if(is.null(types)){types = unique(samples$type)} 
    
    for(type in types){
        s = samples[samples$type==type,]
        if(nrow(s) <= 0){next}
        ns = data.frame(type=type, GFlop.s=median(s$GFlop.s), sd=sd(s$GFlop.s), nthreads=s$n_threads[1], stringsAsFactors=F)
        fpeaks = rbind(fpeaks,ns)
    }
    if(verbose){print(fpeaks); cat("\n")}
    fpeaks
}

bandwidth_roofs <- function(df, types=NULL, verbose=T){
    if(verbose){print("Bandwidths:")}
    bandwidths = data.frame(obj=character(0), type=character(0), GByte.s=numeric(0), sd=numeric(0), nthreads=integer(0), stringsAsFactors=FALSE)
    samples = df[df$GFlop.s==0,]
    if(is.null(types)){
        types = list()
        for(obj in unique(samples$memory)){
            for(type in unique(samples$type)){
                types[[length(types)+1]] = list(obj,type)
            }
        }
    }
    for(type in types){
        s = samples[samples$memory==type[1] & samples$type==type[2],]
        if(nrow(s) <= 0){next}
        ns = data.frame(obj=as.character(type[1]), type=as.character(type[2]), GByte.s=median(s$GByte.s), sd=sd(s$GByte.s), nthreads=s$n_threads[1], stringsAsFactors=F)
        bandwidths = rbind(bandwidths, ns)
    }
    if(verbose){print(bandwidths); cat("\n")}
    bandwidths[order(bandwidths$GByte.s, decreasing=TRUE),]
}

rrmse <-function(y.valid, y.roof){
    y.max = max(y.roof)
    relative_error = (y.valid-y.roof)/y.roof
    relative_root_mean_squared_error = sqrt(sum(relative_error*relative_error))/length(y.valid)
}

validation_points <- function(df, type){
    ret = new.env()
    ret$points = data.frame(AI=numeric(0), 
                            GFlop.s=numeric(0), 
                            sd=numeric(0), 
                            obj=character(0),
                            type=character(0),
                            stringsAsFactors=FALSE)
    
    samples = df[!is.infinite(df$Flops.Byte) & df$Flops.Byte>0 & df$type==type[2] & df$memory==type[1],]
    AIs = unique(samples$Flops.Byte)
    for(AI in AIs){
        s = samples[samples$Flops.Byte==AI,]
        if(nrow(s) <= 0){next}
        ns = data.frame(AI=AI, GFlop.s=median(s$GFlop.s), sd=sd(s$GFlop.s), obj=type[1], type=type[2], stringsAsFactors=FALSE)
        ret$points = rbind(ret$points,ns)
    }
    
    bandwidth = bandwidth_roofs(df, list(type), verbose=F)
    fpeak = max(df$GFlop.s)
    roof = sapply(1:nrow(ret$points), function(i){min(c(bandwidth$GByte.s[1]*ret$points$AI[i], fpeak))})
    ret$error = data.frame(memory=type[1], op_type=type[2], fpeak.GFlop.s=fpeak, error=rrmse(ret$points$GFlop.s, roof), stringsAsFactors=FALSE)
    ## Error of validation points computed with: (sqrt(sum((y-y')/y')^2)/length(y))" 
    
    ret
}

data_points <- function(pt, types=NULL, verbose=T){
    #pts are obtained for a single localion into user data frame.
    if(is.null(pt)){return(NULL)}

    ## Prepare output Frame
    ret = data.frame(AI=numeric(0), 
                     GFlop.s=numeric(0), 
                     sd=numeric(0), 
                     time=numeric(0),                   
                     info=character(0), 
                     type=character(0), 
                     stringsAsFactors=FALSE)


    ## If no type filter provided then enumerate types
    if(is.null(types)){
        types = list()
        for(type in unique(pt$type)){
            for(info in unique(pt$info)){
                types[[length(types)+1]] = list(type,info)
            }
        }
    }

    ## Accumulate samples with same types and location
    for(type in types){
        s = pt[pt$info==type[2] & pt$type==type[1],]
        if(nrow(s) <= 0){next}
        Elapsed.s=sum(as.numeric(s$Nanoseconds))/1e9
        GFlop=sum(as.numeric(s$Flops))/1e9
        GByte=sum(as.numeric(s$Bytes))/1e9
        ns = data.frame(GFlop=GFlop,
                        GByte=GByte,
                        Elapsed.s=Elapsed.s,                        
                        AI=GFlop/GByte, 
                        GFlop.s=GFlop/Elapsed.s,
                        GBytes.s=GByte/Elapsed.s,
                        info=as.character(type[2]), 
                        type=as.character(type[1]), 
                        stringsAsFactors=FALSE)
        ret = rbind(ret, ns)
    }
    
    if(verbose){
        print("Application points:")
        print(ret);
        cat("\n")
    }
    ret
}

#################################################################################################################################
## Plot data
#################################################################################################################################

##Logarithmic sequence of points
lseq <- function(from=1, to=100000, length.out = 6) {
    exp(seq(log(from), log(to), length.out = length.out))
}

require(graphics)

plot_bandwidths <- function(bandwidths, fpeak_max, xlim, ylim){
    ymax=min(c(options$ymax,fpeak_max))
    
    print(options$ymax)
    print(fpeak_max)
    AI = lseq(xlim[1], xlim[2], 500)
    colors=options$color:(options$color+nrow(bandwidths)-1)
    for(i in 1:nrow(bandwidths)){
        ## plot roof
        GByte.s = bandwidths$GByte.s[i]
        GFlop.s = sapply(AI*GByte.s, min, ymax)
        AI_b= AI[GFlop.s<ymax]
        GFlop.s = GFlop.s[GFlop.s<ymax]        
        plot(AI_b, GFlop.s, lty=1, type="l", log="xy", axes=FALSE, xlab="", ylab="", xlim=xlim, ylim=ylim, col=colors[i])

        ## print label on roofs
        if(display_labels()){
            ymax=min(c(ymax, options$ymax))
            if(ymax > AI[length(AI)]*GByte.s){
                ymax = AI[length(AI)]*GByte.s; xmax = AI[length(AI)]            
            } else {
                xmax = ymax/GByte.s*0.9
            }
            ##Compute label positions
            if(GByte.s*AI[1] > ylim[1]){ xmin = AI[1]; ymin = GByte.s*AI[1] } else { ymin = ylim[1]; xmin = ymin/GByte.s }
            if(options$labels == "left"){ x=xmin; y=ymin*1.3; pos=4 } else { x=xmax; y=ymax; pos=2 }

            ##compute angle
            label = sprintf("%s_%s", bandwidths$obj[i], bandwidths$type[i])
            dy    = grconvertY(ymax, from = "user", to = "device") - grconvertY(ymin, from = "user", to = "device")
            dx    = grconvertX(xmax, from = "user", to = "device") - grconvertX(xmin, from = "user", to = "device")
            srt   <<- atan(dy/dx)*180/pi
            
            ## display label only if does not overlap another label
            dmax = log(ylim[2])-log(ylim[1])
            if(i == 1){
                dcurr = dmax
            } else {
                dcurr = log(xlim[1]*bandwidths$GByte.s[i-1]) - log(xlim[1]*GByte.s)
            }
            if(dcurr > dmax/20){
                text(x=x, y=y,
                     labels=sprintf("%s_%s  ",bandwidths$obj[i], bandwidths$type[i]),
                     srt=srt,
                     cex=.7*options$cex,
                     col="darkgrey",
                     pos=pos)
            }
        }

        ## plot deviation
        if(options$stats){
            sd = bandwidths$sd[i]
            a0.x = AI[1];                  a0.y = AI[1]*(GByte.s-sd*0.5)
            a1.x = AI[1];                  a1.y = AI[1]*(GByte.s+sd*0.5)
            a2.x = ymax/(GByte.s+sd*0.5); a2.y = ymax
            a3.x = ymax/(GByte.s-sd*0.5); a3.y = ymax
            coord.x = c(a0.x, a1.x, a2.x, a3.x)
            coord.y = c(a0.y, a1.y, a2.y, a3.y)
            par(new=TRUE)            
            polygon(coord.x,coord.y,col=adjustcolor(options$color+i-1,alpha.f=.25), lty="blank")
        }
        
        ## print legend
        if(display_legend() && !options$validation && is.null(options$data)){
            legend=sprintf("%s_%s = %.2fGByte/s",
                           bandwidths$obj,
                           bandwidths$type,
                           bandwidths$GByte.s)
            legend(options$legend,
                   legend=legend,
                   lty=1,
                   col=colors,
                   bg="white",
                   cex=.7*options$cex)
        }

        par(new=T)        
    }
}

plot_fpeaks <- function(fpeaks, xmin){
    for(i in 1:nrow(fpeaks)){
        abline(h = fpeaks$GFlop.s[i], lty=3, lwd=1.5, col="darkgrey");
        text(x=xmin, y=fpeaks$GFlop.s[i], labels=fpeaks$type[i], adj=c(0,1), cex=.7*options$cex, col="darkgrey")
        par(new=T)
    }
}

plot_validation <-function(pts, col=1, pch=1){
    points(pts$AI, pts$GFlop.s, pch=pch, col=col, cex=.5*options$cex)
    if(options$stats){
        segments(x0 = pts$AI, x1 = pts$AI, y0 = pts$GFlop.s-pts$sd*0.5, y1=pts$GFlop.s+pts$sd*0.5, col=col, lty=1)
    }
}


label_Ycoords <- function(pts, ylim, frac=50){
    Y    = log(pts$GFlop.s,10)
    n    = length(Y)
    dmin = (log(ylim[2],10) - log(ylim[1],10))/frac
    
    distance_matrix <- function(Y){
        n = length(Y); m = matrix(nrow=n, ncol=n)
        for(i in 1:n){ for(j in 1:i){ m[i,j] = m[j,i] = abs(Y[j]-Y[i]) } }
        m
    }

    dist = distance_matrix(Y)
    for(i in 2:n){
        if(dist[i,i-1] < dmin){
            Y[i]=Y[i-1]+dmin;
            dist = distance_matrix(Y)
        }
    }
    
    Y = 10^Y
    Y
}

plot_data <-function(pts, xlim, ylim, col=1, labels=NULL, rot=0){
    if(length(col) == 1){ colors = (col):(col+nrow(pts)-1) } else{ colors=col }
    
    lgd = sapply(1:nrow(pts), function(i){sprintf("%s", pts$info[i])})

    ##sort points
    ind = order(pts$GFlop.s)
    pts = pts[ind,]

    ##plot points and text
    points(pts$AI, pts$GFlop.s, pch=colors, col=colors, cex=options$cex)

    ##plot circle around point
    ## tmax = max(pts$Elapsed.s)
    ## for(i in 1:nrow(pts)){
    ##     r = 0.05 #pts$Elapsed.s[i]*0.1/tmax
    ##     symbols(pts$AI[i],
    ##             pts$GFlop.s[i],
    ##             circles=1,
    ##             inches=r,
    ##             add=T,
    ##             fg=colors[i],
    ##             bg=adjustcolor(colors[i],alpha.f=.25),
    ##             xlim=xlim,
    ##             ylim=ylim, lwd=.5)
    ## }
    
    ##plot labels
    if(is.null(labels) || length(labels) != nrow(pts)){
        labels = pts$info
    } else {
        labels = labels[ind]
    }
    if(nrow(pts) > 1){ ylab_coord = label_Ycoords(pts,ylim,40) } else { ylab_coord = pts$GFlop.s }
    text(pts$AI, ylab_coord,  labels=labels, cex=.6*options$cex, col="black", pos=2, srt=0)
    par(new=T)    
}

plot_data_group <- function(pts, xlim, ylim, col){
    groups = toupper(unlist(strsplit(options$group, ",")))
    AIs = vector("numeric", length=length(groups))
    x = vector("numeric", length=length(groups))

    remove = replicate(nrow(pts), FALSE)
    for(i in 1:length(groups)){
        matching = grepl(groups[i], pts$info, perl=TRUE)
        elements = pts[matching,]
        remove = remove | matching
        labels = gsub(pattern=groups[i], replacement="", x=elements$info, perl=TRUE)
        AIs[i] = mean(elements$AI)
        color = replicate(nrow(elements), (col+i-1))
        plot_data(elements, xlim, ylim, color, labels, rot=srt)
    }

    ##abline(v=AIs, col="darkgrey", lty=2)
    ##text(AIs, ylim[1], labels=groups, cex=0.7*options$cex, srt=-90, pos=2, offset=-0.4)
    if(display_legend()){
        legend = sprintf("%s AI=%.2f", groups, AIs)
        colors = (col):(col+length(groups)-1)
        legend(options$legend,
               legend=legend,
               col=colors,
               pch=colors,
               bg="white",
               cex=.7*options$cex)
    }
    
    ## if(xlim[2]>2*xlim[1]){
    ##     ticks = which(abs(log(AIs,2) - round(log(AIs,2))) > 0.1)
    ##     if(length(ticks) > 0){
    ##         axis(1, labels=FALSE, at=AIs[ticks])        
    ##         text(x=AIs[ticks],
    ##              y=10^(par("usr")[3]),
    ##              pos=1,
    ##              xpd=NA,
    ##              labels=sprintf("%.3f      ",AIs[ticks]),
    ##              srt=45,
    ##              cex=.9*options$cex,
    ##              offset=1)
    ##     }
    ## }
    
    if(length(which(!remove))>0){ plot_data(pts[-remove,], xlim, ylim, col=length(groups)+1) }
}

roofline_plot <- function(df, location, bandwidths, fpeaks, validation=F, data=NULL, verbose=T, plotx=TRUE, ploty=TRUE){
    ##Set limit and ticks
    zoom = unlist(strsplit(options$zoom, split=","))
    xmin = as.numeric(zoom[1]); xmax = as.numeric(zoom[2]); xlim = c(xmin,xmax)
    if(xmax/xmin > 2){
        xticks = lseq(xmin,xmax,log(xmax/xmin,base=2) + 1)
        xlabels = sapply(xticks, function(i) as.expression(bquote(2^ .(round(log(i,base=2))))))
    } else {
        xticks = seq(xmin, xmax, (xmax-xmin)/10)
        xlabels = sapply(xticks, function(i) sprintf("%.3f", i))
    }

    ##Set ymax bounds
    fpeak_max = max(fpeaks$GFlop.s)
    if(options$ymax<=0){
        options$ymax <<- min(c(fpeak_max, max(bandwidths$GByte.s)*xmax))
    }
##    options$ymax = 10^ceiling(log10(fpeak_max))
    ymin = options$ymin; ylim = c(ymin,options$ymax)
    
    ytick_min = 10^ceiling(log10(ymin))
    if(abs( log10(ytick_min) - log10(ymin) ) < 0.1){ytick_min = ytick_min *10}
    ytick_max = 10^floor(log10(options$ymax))
    pow_ticks = lseq(ytick_min, ytick_max, log10(ytick_max/ytick_min)+1)
    yticks    = unlist(c(ymin, pow_ticks, options$ymax))
    ylabels   = unlist(c(sprintf("%.2f", ymin),
                         sapply(pow_ticks, function(i) as.expression(bquote(10^ .(round(log10(i)))))),
                         sprintf("%.2f", options$ymax)))
        
    ##Plot grid
    plot(1, type="n", axes=F, xlab="", ylab="", xlim=xlim, ylim=ylim, log="xy")
    if(length(yticks) <= 4){
        abline(h=c(yticks, lseq(min(yticks), max(yticks), 10)), v=xticks, col="darkgray", lty = 3)
    } else {
        abline(h=yticks, v=xticks, col="darkgray", lty = 3)
    }
    
    par(new=T)
        
    ##plot validation points
    if(validation){
        errors = data.frame(memory=character(0), op_type=character(0), fpeak.GFlop.s=numeric(0), error=numeric(0), stringsAsFactors=FALSE)
        for(i in 1:nrow(bandwidths)){
            valid = validation_points(df, c(bandwidths$obj[i], bandwidths$type[i]))
            plot_validation(valid$points, col=i)
            par(new=T)
            errors = rbind(errors, valid$error)
        }
        if(verbose){
            print("Model fitness:")        
            print(errors)
            cat("\n")
       } 
        bandwidths = cbind(bandwidths, errors)        
        if(display_legend() && is.null(options$data)){
            colors=1:nrow(bandwidths)
            legend=sprintf("%s_%s error = %.1f%%",
                           bandwidths$obj,
                           bandwidths$type,
                           100*bandwidths$error)
            legend(options$legend,
                   legend=legend,
                   lty=1,
                   col=colors,
                   bg="white",
                   cex=.7*options$cex)
            par(new=T)
        }
    }

    ##plot bandwidths
    plot_bandwidths(bandwidths, fpeak_max, xlim, ylim)
    
    ##plot layout
    if(plotx){ axis(1, at=xticks, labels=xlabels, cex.axis=options$cex) }

    ##plot fpeaks
    plot_fpeaks(fpeaks,xmin)
    
    fpeaks.labels = fpeaks$GFlop.s
    for(label in fpeaks$GFlop.s){
        for(tick in yticks){
            ytick = as.numeric(tick)
            ylabel = as.numeric(label)
            if(abs(ylabel-ytick) < ytick){
                rm = which(yticks==ytick)
                yticks  = yticks[-rm]
                ylabels = ylabels[-rm]                
            }
        }
    }
    yticks = c(yticks, fpeaks$GFlop.s)
    ylabels = c(ylabels, sprintf("%.1f", as.numeric(fpeaks$GFlop.s)))    
    if(ploty){ axis(2, at=yticks, labels=ylabels, las=1, cex.axis=options$cex) }
    
    ##plot data points
    if(!is.null(data)){
        color = nrow(bandwidths)+options$color-2
        if(!is.null(options$group)){
            plot_data_group(data, xlim, ylim, col=color)
        } else {
            plot_data(data, xlim, ylim, col=color)
        }
    }
    if(!options$nosubtitle){ title(main=location, cex.main=.9*options$cex) }
}


height=6
width=height/options$ar
pdf(options$output, onefile=T, family = "Helvetica", basename(options$input), width=width, height=height)

outer=TRUE
nr = 1
nc = length(locations)
layout(matrix(1:(nr*nc), nrow=nr, ncol=nc))
bom=3; lom=5; tom=2; rom=0
tim=1; bim=1; lim=0; rim=0
par(oma=c(bom,lom,tom,rom), mar=c(bim,lim,tim,rim))

if(length(locations) == 1){
    if(options$nosubtitle){
        outer=FALSE
        if(is.null(options$title)){
            tom=0; bom=2.8; lom=6.2; tim=0; rim=0.1
            par(oma=c(bom,lom,tom,rom), mar=c(bim,lim,tim,rim))
        } else {
            tom=1;
            par(oma=c(bom,lom,tom,rom), mar=c(bim,lim,tim,rim))
        }
    } else {
        lom=5.7; tom=0.5; rom=0.1
        par(oma=c(bom,lom,tom,rom), mar=c(bim,lim,tim,rim))
    }    
}

for(i in 1:length(locations)){
    if(i <= nr){ploty=TRUE} else {ploty=FALSE}
    if(i >= (nr-1)*nc){plotx=TRUE} else {plotx=FALSE}    
    loc = locations[i]
    cat(sprintf("Process location: %s", loc), "\n\n")
    roofs = df[df$location==loc,]
    appli = NULL; if(!is.null(pt)){appli = pt[pt$Location==loc,]}
    bandwidths = bandwidth_roofs(roofs, types=bandwidth_types)
    fpeaks = fpeak_roofs(roofs, types=fpeak_types)
    roofline_plot(roofs,
                  loc,
                  bandwidths,
                  fpeaks,
                  options$validation,
                  data_points(appli),
                  plotx=plotx,
                  ploty=ploty)
}


title(main=options$title, outer=outer, cex.main=1.1*options$cex)
title(xlab="Flops/Byte", ylab="GFlop/s\n\n", outer=TRUE, cex.lab=options$cex, mgp=c(1.5,0,3))

graphics.off()
cat(sprintf("Output to %s", options$output), "\n")

