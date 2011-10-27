package com.waze.widget;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Paint.Align;

import org.achartengine.ChartFactory;
import org.achartengine.chart.PointStyle;
import org.achartengine.model.TimeSeries;
import org.achartengine.model.XYMultipleSeriesDataset;
import org.achartengine.renderer.XYMultipleSeriesRenderer;
import org.achartengine.renderer.XYSeriesRenderer;
import org.achartengine.util.MathHelper;

import com.waze.widget.config.WazeLang;
import com.waze.widget.routing.RouteScore;
import com.waze.widget.routing.RouteScoreType;
import com.waze.widget.routing.RoutingRequest;
import com.waze.widget.routing.RoutingResponse;

public class WazeAppWidgetChart {
   
  private static final long MINUTE = 60 * 1000;

  /*************************************************************************************************
    * Builds an XY multiple series renderer.
    * 
    * @param colors the series rendering colors
    * @param styles the series point styles
    * @return the XY multiple series renderers
    */
   protected XYMultipleSeriesRenderer buildRenderer(int[] colors, PointStyle[] styles) {
     XYMultipleSeriesRenderer renderer = new XYMultipleSeriesRenderer();
     renderer.setAxisTitleTextSize(20);
     renderer.setChartTitleTextSize(20);
     renderer.setLabelsTextSize(20);
     renderer.setLegendTextSize(1520);
     renderer.setPointSize(5f);
     renderer.setMargins(new int[] { 20, 30, 15, 0 });
     int length = colors.length;
     for (int i = 0; i < length; i++) {
       XYSeriesRenderer r = new XYSeriesRenderer();
       r.setColor(colors[i]);
       r.setPointStyle(styles[i]);
       renderer.addSeriesRenderer(r);
     }
     return renderer;
   }
   
   
   /*************************************************************************************************
    * Sets a few of the series renderer settings.
    * 
    * @param renderer the renderer to set the properties to
    * @param title the chart title
    * @param xTitle the title for the X axis
    * @param yTitle the title for the Y axis
    * @param xMin the minimum value on the X axis
    * @param xMax the maximum value on the X axis
    * @param yMin the minimum value on the Y axis
    * @param yMax the maximum value on the Y axis
    * @param axesColor the axes color
    * @param labelsColor the labels color
    */
   protected void setChartSettings(XYMultipleSeriesRenderer renderer, String title, String xTitle,
       String yTitle, double xMin, double xMax, double yMin, double yMax, int axesColor,
       int labelsColor) {
     renderer.setChartTitle(title);
     renderer.setXTitle(xTitle);
     renderer.setYTitle(yTitle);
     renderer.setXAxisMin(xMin);
     renderer.setXAxisMax(xMax);
     renderer.setYAxisMin(yMin);
     renderer.setYAxisMax(yMax);
     renderer.setAxesColor(axesColor);
     renderer.setLabelsColor(labelsColor);
   }
   
   /*************************************************************************************************
    * Builds an XY multiple time dataset using the provided values.
    * 
    * @param titles the series titles
    * @param xValues the values for the X axis
    * @param yValues the values for the Y axis
    * @return the XY multiple time dataset
    */
   protected XYMultipleSeriesDataset buildDateDataset(String[] titles, List<Date[]> xValues,
		      List<double[]> yValues) {
		    XYMultipleSeriesDataset dataset = new XYMultipleSeriesDataset();
		    int length = titles.length;
		    for (int i = 0; i < length; i++) {
		      TimeSeries series = new TimeSeries(titles[i]);
		      Date[] xV = xValues.get(i);
		      double[] yV = yValues.get(i);
		      int seriesLength = xV.length;
		      for (int k = 0; k < seriesLength; k++) {
		        //WazeAppWidgetLog.d( "buildDateDataset title=["+titles[i]+"] xV[k]="+xV[k]+" yV[k]="+yV[k]+" length ="+seriesLength);
		        series.add(xV[k], yV[k]);
		      }
		      dataset.addSeries(series);
		    }
		    return dataset;
		  }
   
	/*************************************************************************************************
    * execute
    * 
    */
   public Intent execute(Context context, RoutingResponse rsp, long timeStamp) {
	    String[] titles = new String[] { "Now","NowTraingle", "ALL","Red", "Orange","Green" };
	    long now = (new Date().getTime()) ;///(30*60*1000))*(30*60*1000) ;
	    Date nowStr = new Date(now);
	    WazeAppWidgetLog.d( "WidgetChart execute timeStamp=: " +  timeStamp);
	    
	    
	    List<Date[]> x = new ArrayList<Date[]>();
	    // Now highlighted
	    Date[] now_dates = new Date[2];
	    now_dates[0] = new Date(now  );
	    now_dates[1] = new Date(now + 1);
	    x.add(now_dates);
	    
	    // Now Trainagle
	    Date[] NowTraingle = new Date[1];
	    NowTraingle[0] = new Date(now);
	    x.add(NowTraingle);

	    
	    for (int i = 1; i < titles.length; i++) {
	      Date[] dates = new Date[rsp.getNumResults()]; 
	      for (int j = 0; j < rsp.getNumResults(); j++) {
	    	  dates[j] = new Date(timeStamp + (j * 10 * MINUTE) - 10 * MINUTE * RoutingRequest.getNowLocation());
	      }
	      x.add(dates);
	      
	    }
	    
	    List<double[]> values = new ArrayList<double[]>();

	    double AllArray[] =  (double[])rsp.getList();	    
   	
	    int seriesLength = AllArray.length;
    	double  RedArray[] = new double[seriesLength];
    	double  NowArray[] = new double[2];
    	double  NowTraiangleArray[] = new double[1];
    	double  OrangeArray[]= new double[seriesLength];
    	double GreenArray[] = new double[seriesLength];
	    for (int k = 0; k < seriesLength; k++) {
	    	RedArray[k] = MathHelper.NULL_VALUE;
	    	GreenArray[k] = MathHelper.NULL_VALUE;
	    	OrangeArray[k] = MathHelper.NULL_VALUE;
	    	if (AllArray[k] != MathHelper.NULL_VALUE ){
		    	RouteScoreType score =RouteScore.getScore((int)AllArray[k] ,rsp.getAveragetTime()/60); 
		    	if ( score == RouteScoreType.ROUTE_GOOD){
		    		GreenArray[k] = AllArray[k];
		    	}else if (score == RouteScoreType.ROUTE_BAD){
		    		RedArray[k] = AllArray[k];
		    	}else{
		    		OrangeArray[k] = AllArray[k];
		    	}
	    	}	    	
	    }
	      
	    NowArray[0] = rsp.getMaxValue()/60+100;
	    NowArray[1] = rsp.getMinValue()/60-100; 
	    values.add(NowArray);
	    
	    NowTraiangleArray[0] = rsp.getMinValue()/60-9;
	    values.add(NowTraiangleArray);
	    
	    values.add(AllArray);
	    values.add(RedArray);
	    values.add(OrangeArray);
	    values.add(GreenArray);


	    int[] colors = new int[] { Color.parseColor("#77676767"), Color.RED, Color.WHITE, Color.parseColor("#d62525"), Color.parseColor("#ea8124"), Color.parseColor("#5f8e44")};
	    PointStyle[] styles = new PointStyle[] { PointStyle.DIAMOND,  PointStyle.TRIANGLE, PointStyle.CIRCLE , PointStyle.CIRCLE,  PointStyle.CIRCLE,  PointStyle.CIRCLE };
	    XYMultipleSeriesRenderer renderer = buildRenderer(colors, styles);
	    int length = renderer.getSeriesRendererCount();
	    for (int i = 0; i < length; i++) {
	      ((XYSeriesRenderer) renderer.getSeriesRendererAt(i)).setFillPoints(true);
	    }

	    ((XYSeriesRenderer) renderer.getSeriesRendererAt(0)).setLineWidth(30);
	    ((XYSeriesRenderer) renderer.getSeriesRendererAt(3)).setLineWidth(0);
	    
	    ((XYSeriesRenderer) renderer.getSeriesRendererAt(2)).setLineWidth(7);
	    ((XYSeriesRenderer) renderer.getSeriesRendererAt(3)).setLineWidth(0);
	    ((XYSeriesRenderer) renderer.getSeriesRendererAt(4)).setLineWidth(0);
	    ((XYSeriesRenderer) renderer.getSeriesRendererAt(5)).setLineWidth(0);

	    long minX = ((x.get(2)[0].getTime() - 10 * MINUTE));
	    long maxX = ((x.get(2)[rsp.getNumResults() - 1].getTime() + 10 * MINUTE));
	    long minY =  (long) ( (double) rsp.getMinValue()/60*0.9 );
	    long maxY =  (long) ( (double) rsp.getMaxValue()/60*1.1 );
	    setChartSettings(renderer, WazeLang.getLang("Your Commute-O-Meter"), "Time", "Min", minX, maxX, minY, maxY, Color.LTGRAY, Color.LTGRAY);
	    
	    renderer.setXLabels(7);
	    renderer.setYLabels(8);
	    renderer.setAntialiasing(true);
	    renderer.setPointSize(11);
	    renderer.setShowGrid(true);
	    renderer.setChartTitleTextSize(26);
	    renderer.setShowLegend(false);
	    renderer.setMargins(new int[] {50, 60, 90, 40});
	    renderer.setXLabelsAlign(Align.CENTER);
	    renderer.setYLabelsAlign(Align.RIGHT);
	    Intent intent = ChartFactory.getTimeChartIntent(context, buildDateDataset(titles, x, values),
	        renderer, "h:mm");
	    return intent;
	  }
}