import org.apache.spark.SparkConf;
import org.apache.spark.api.java.JavaRDD;
import org.apache.spark.api.java.JavaPairRDD;
import org.apache.spark.api.java.JavaSparkContext;
import org.apache.spark.api.java.function.*;
import scala.Tuple2;

import java.util.*;

/*
 * Main class of the TFIDF Spark implementation.
 * Author: Hari Krishna Majety
 * Date:   11/9/2018
 */
public class TFIDF {

	static boolean DEBUG = true;

    public static void main(String[] args) throws Exception {
        // Check for correct usage
        if (args.length != 1) {
            System.err.println("Usage: TFIDF <input dir>");
            System.exit(1);
        }
		
		// Create a Java Spark Context
		SparkConf conf = new SparkConf().setAppName("TFIDF");
		JavaSparkContext sc = new JavaSparkContext(conf);

		// Load our input data
		// Output is: ( filePath , fileContents ) for each file in inputPath
		String inputPath = args[0];
		JavaPairRDD<String,String> filesRDD = sc.wholeTextFiles(inputPath);
		
		// Get/set the number of documents (to be used in the IDF job)
		long numDocs = filesRDD.count();
		
		//Print filesRDD contents
		if (DEBUG) {
			List<Tuple2<String, String>> list = filesRDD.collect();
			System.out.println("------Contents of filesRDD------");
			for (Tuple2<String, String> tuple : list) {
				System.out.println("(" + tuple._1 + ") , (" + tuple._2.trim() + ")");
			}
			System.out.println("--------------------------------");
		}
		
		/* 
		 * Initial Job
		 * Creates initial JavaPairRDD from filesRDD
		 * Contains each word@document from the corpus and also attaches the document size for 
		 * later use
		 * 
		 * Input:  ( filePath , fileContents )
		 * Map:    ( (word@document) , docSize )
		 */
		JavaPairRDD<String,Integer> wordsRDD = filesRDD.flatMapToPair(
			new PairFlatMapFunction<Tuple2<String,String>,String,Integer>() {
				public Iterable<Tuple2<String,Integer>> call(Tuple2<String,String> x) {
					// Collect data attributes
					String[] filePath = x._1.split("/");
					String document = filePath[filePath.length-1];
					String fileContents = x._2;
					String[] words = fileContents.split("\\s+");
					int docSize = words.length;
					
					// Output to Arraylist
					ArrayList ret = new ArrayList();
					for(String word : words) {
						ret.add(new Tuple2(word.trim() + "@" + document, docSize));
					}
					return ret;
				}
			}
		);
		
		//Print wordsRDD contents
		if (DEBUG) {
			List<Tuple2<String, Integer>> list = wordsRDD.collect();
			System.out.println("------Contents of wordsRDD------");
			for (Tuple2<String, Integer> tuple : list) {
				System.out.println("(" + tuple._1 + ") , (" + tuple._2 + ")");
			}
			System.out.println("--------------------------------");
		}		
		
		/* 
		 * TF Job (Word Count Job + Document Size Job)
		 * Gathers all data needed for TF calculation from wordsRDD
		 *
		 * Input:  ( (word@document) , docSize )
		 * Map:    ( (word@document) , (1/docSize) )
		 * Reduce: ( (word@document) , (wordCount/docSize) )
		 */
		JavaPairRDD<String,String> tfRDD = wordsRDD.mapValues(
			f -> "1/"+ f).reduceByKey( 
			new Function2<String,String,String>() {
				public String call(String value1, String value2) {

					String[] value1Splitted = value1.split("/");
					String[] value2Splitted = value2.split("/");

					Integer value1Numerator = Integer.parseInt(value1Splitted[0]);
					Integer value2Numerator = Integer.parseInt(value2Splitted[0]);
					//Add the Numerators
					String wordCount = Integer.toString(value1Numerator+value2Numerator);
					String retVal = wordCount+"/"+value1Splitted[1];
					return (retVal);
				}
			}
		);
		
		//Print tfRDD contents
		if (DEBUG) {
			List<Tuple2<String, String>> list = tfRDD.collect();
			System.out.println("-------Contents of tfRDD--------");
			for (Tuple2<String, String> tuple : list) {
				System.out.println("(" + tuple._1 + ") , (" + tuple._2 + ")");
			}
			System.out.println("--------------------------------");
		}
		
		/*
		 * IDF Job
		 * Gathers all data needed for IDF calculation from tfRDD
		 *
		 * Input:  ( (word@document) , (wordCount/docSize) )
		 * Map:    ( word , (1/document) )
		 * Reduce: ( word , (numDocsWithWord/document1,document2...) )
		 * Map:    ( (word@document) , (numDocs/numDocsWithWord) )
		 */
		JavaPairRDD<String,String> idfRDD = tfRDD.mapToPair(
			
			new PairFunction<Tuple2<String,String>,String,String>() {
				public Tuple2<String,String> call(Tuple2<String,String> x) {
					String word = x._1.split("@")[0];
					String document = x._1.split("@")[1];
					//double TF = wordCount/docSize;
					//Form the string as per the output requirement we defined above
					return new Tuple2(word, "1/"+document);
				}
			}
			
		).reduceByKey(
			
			/************ YOUR CODE HERE ************/
			new Function2<String,String,String>() {
				public String call(String value1, String value2) {
					String[] value1Splitted = value1.split("/");
					String[] value2Splitted = value2.split("/");
					Integer value1Numerator = Integer.parseInt(value1Splitted[0]);
					Integer value2Numerator = Integer.parseInt(value2Splitted[0]);
					//Add the Numerators
					String numDocsWithWord = Integer.toString(value1Numerator+value2Numerator);
					//Form the string as per the output requirement we defined above
					String retVal = numDocsWithWord+"/"+value1Splitted[1] +","+ value2Splitted[1];
					return (retVal);
				}
			}
			
		).flatMapToPair(
			
			/************ YOUR CODE HERE ************/
			new PairFlatMapFunction<Tuple2<String,String>,String, String>() {
				public Iterable<Tuple2<String,String>> call(Tuple2<String,String> x) {
					// Collect data attributes
					String[] value1Splitted = x._2.split("/");
					String[] docsList = value1Splitted[1].split(",");
					String numDocsWithWord = value1Splitted[0];
					/*String document = filePath[filePath.length-1];
					String fileContents = x._2;
					String[] words = fileContents.split("\\s+");
					int docSize = words.length;*/
					String numDocsString = Long.toString(numDocs);
					// Output to Arraylist
					ArrayList ret = new ArrayList();
					for(String doc : docsList) {
						//Form the string as per the output requirement we defined above
						ret.add(new Tuple2( x._1+ "@" +doc ,numDocsString + "/"  + numDocsWithWord));
					}
					return ret;
				}
			}
		);
		
		//Print idfRDD contents
		if (DEBUG) {
			List<Tuple2<String, String>> list = idfRDD.collect();
			System.out.println("-------Contents of idfRDD-------");
			for (Tuple2<String, String> tuple : list) {
				System.out.println("(" + tuple._1 + ") , (" + tuple._2 + ")");
			}
			System.out.println("--------------------------------");
		}
	
		/*
		 * TF * IDF Job
		 * Calculates final TFIDF value from tfRDD and idfRDD
		 *
		 * Input:  ( (word@document) , (wordCount/docSize) )          [from tfRDD]
		 * Map:    ( (word@document) , TF )
		 * 
		 * Input:  ( (word@document) , (numDocs/numDocsWithWord) )    [from idfRDD]
		 * Map:    ( (word@document) , IDF )
		 * 
		 * Union:  ( (word@document) , TF )  U  ( (word@document) , IDF )
		 * Reduce: ( (word@document) , TFIDF )
		 * Map:    ( (document@word) , TFIDF )
		 *
		 * where TF    = wordCount/docSize
		 * where IDF   = ln(numDocs/numDocsWithWord)
		 * where TFIDF = TF * IDF
		 */
		JavaPairRDD<String,Double> tfFinalRDD = tfRDD.mapToPair(
			new PairFunction<Tuple2<String,String>,String,Double>() {
				public Tuple2<String,Double> call(Tuple2<String,String> x) {
					double wordCount = Double.parseDouble(x._2.split("/")[0]);
					double docSize = Double.parseDouble(x._2.split("/")[1]);
					double TF = wordCount/docSize;
					return new Tuple2(x._1, TF);
				}
			}
		);
		
		JavaPairRDD<String,Double> idfFinalRDD = idfRDD.mapToPair(
			
			/************ YOUR CODE HERE ************/
			new PairFunction<Tuple2<String,String>,String,Double>() {
				public Tuple2<String,Double> call(Tuple2<String,String> x) {
					double numDocs = Double.parseDouble(x._2.split("/")[0]);
					double numDocsWithWord = Double.parseDouble(x._2.split("/")[1]);
					//Compute the IDF
					double IDF = Math.log((double)numDocs/numDocsWithWord);
					return new Tuple2(x._1, IDF);
				}
			}
			
		);
		
		JavaPairRDD<String,Double> tfidfRDD = tfFinalRDD.union(idfFinalRDD).reduceByKey(
			
			/************ YOUR CODE HERE ************/
			new Function2<Double,Double,Double>() {
				public Double call(Double value1, Double value2) {
					/*String[] value1Splitted = value1.split("/");
					String[] value2Splitted = value2.split("/");
					Integer value1Numerator = Integer.parseInt(value1Splitted[0]);
					Integer value2Numerator = Integer.parseInt(value2Splitted[0]);
					String numDocsWithWord = Integer.toString(value1Numerator*value2Numerator);
					String retVal = numDocsWithWord+"/"+value1Splitted[1] +","+ value2Splitted[1];*/

					//Compute the TFIDF by multiplying TF and IDF part
					Double retVal = value1*value2;
					return (retVal);
				}
			}

			
		).mapToPair(
			
			/************ YOUR CODE HERE ************/
			new PairFunction<Tuple2<String,Double>,String,Double>() {
				public Tuple2<String,Double> call(Tuple2<String,Double> x) {
					/*double wordCount = Double.parseDouble(x._2.split("/")[0]);
					double docSize = Double.parseDouble(x._2.split("/")[1]);
					double TF = wordCount/docSize;*/
					String[] keyParts = x._1.split("@");
					return new Tuple2(keyParts[1]+"@"+keyParts[0], x._2);
				}
			}
			
		);
		
		//Print tfidfRDD contents in sorted order
		Map<String, Double> sortedMap = new TreeMap<>();
		List<Tuple2<String, Double>> list = tfidfRDD.collect();
		for (Tuple2<String, Double> tuple : list) {
			sortedMap.put(tuple._1, tuple._2);
		}
		if(DEBUG) System.out.println("-------Contents of tfidfRDD-------");
		for (String key : sortedMap.keySet()) {
			System.out.println(key + "\t" + sortedMap.get(key));
		}
		if(DEBUG) System.out.println("--------------------------------");	 
	}	
}