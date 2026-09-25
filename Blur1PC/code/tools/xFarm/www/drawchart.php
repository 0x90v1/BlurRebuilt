<?php

	include("Image/Graph.php");
    include('secstotimestring.php');
    
function ConvertSecondsToHours($value)
{
	return (int)($value / (60*60));
}

	// create the graph
	$Graph =& new Image_Graph(500, 300);

    // add a TrueType font
    $Arial =& $Graph->addFont(new Image_Graph_Font_TTF("arial.ttf"));
    // set the font size to 15 pixels
    $Arial->setSize(11);
    // add a title using the created font

    $graphName = $_GET['name'];
    
    $mode = $_GET['mode'];

	// create the plotarea
	$Graph->add(
        new Image_Graph_Layout_Vertical(
            new Image_Graph_Title($graphName, $Arial),
            $Plotarea = new Image_Graph_Plotarea(),
            5
        )
    );

    $iMaxValue = 0;

	// create the dataset
	$Datasets =
	    array(
	        new Image_Graph_Dataset_Trivial(),
	        new Image_Graph_Dataset_Trivial()
	        );

    // the data is passed in from the URL
	$data = $_GET['data1'];
	$dataSet1 = explode(",", $data);
	$data = $_GET['data2'];
	$dataSet2 = explode(",", $data);

    $i = 1;
    foreach( $dataSet1 as $key => $value)
    {
		$val2 = $dataSet2[$key];
    	$Datasets[0]->addPoint($i, $value);
    	$Datasets[1]->addPoint($i, $val2);

        $i++;

		$combined = $value + $val2;
    	if ($combined > $iMaxValue)
            $iMaxValue = $combined;
	}

//  $Dataset =& new Image_Graph_Dataset_Random(24,0,10000);

	// create the plot as bar chart using the dataset
	$Plot =& $Plotarea->addPlot(new Image_Graph_Plot_Stacked_Bar($Datasets));

    // set a line color
    $Plot->setLineColor(IMAGE_GRAPH_GRAY);
    // set a standard fill style
    $BLUE =& $Graph->newColor(IMAGE_GRAPH_BLUE, 100);
	$Translucent =& $Graph->newColor(IMAGE_GRAPH_ORANGE, 255);
	
    $AxisY =& $Plotarea->getAxis(IMAGE_GRAPH_AXIS_Y);
    if ($iMaxValue < 60 * 60)
        $AxisY->forceMaximum( 60 * 60 ); // force minimum of one hour's worth of seconds on the Y axis

	if ($mode == 1)
		$AxisY->setDataPreprocessor(new Image_Graph_DataPreprocessor_Function("ConvertSecondsToHours"));
	else
	    $AxisY->setDataPreprocessor(new Image_Graph_DataPreprocessor_Function("ConvertSecondsToTimeString"));

    $AxisX =& $Plotarea->getAxis(IMAGE_GRAPH_AXIS_X);
    $AxisX->showLabel(IMAGE_GRAPH_LABEL_ZERO | IMAGE_GRAPH_LABEL_MINIMUM | IMAGE_GRAPH_LABEL_MAXIMUM);

    $MyFillstyle =& new Image_Graph_Fill_Gradient(IMAGE_GRAPH_GRAD_VERTICAL, IMAGE_GRAPH_BLUE, IMAGE_GRAPH_RED);
//	$Plot->setFillStyle($MyFillstyle);

   // create a fill array
    $BLUE =& $Graph->newColor(IMAGE_GRAPH_BLUE, 100);
    $RED =& $Graph->newColor(IMAGE_GRAPH_RED, 200);

	$FillArray =& new Image_Graph_Fill_Array();
    $FillArray->add($RED);
    $FillArray->add($BLUE);
	$Plot->setFillStyle($FillArray);
//	$BGimage =& new Image_Graph_Fill_Image( "images/chartbg1.png" );
//	$Graph->setBackground($BGimage);

    // output the Graph
	$Graph->done();
?>
