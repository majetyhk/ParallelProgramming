import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.util.GenericOptionsParser;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.mapreduce.lib.input.FileSplit;

import java.io.IOException;
import java.util.*;

/*
 * Main class of the TFIDF MapReduce implementation.
 * Author: Hari Krishna Majety
 * Date:   10/28/2018
 */
public class TFIDF {

    public static void main(String[] args) throws Exception {
        // Check for correct usage
        if (args.length != 1) {
            System.err.println("Usage: TFIDF <input dir>");
            System.exit(1);
        }
		
		// Create configuration
		Configuration conf = new Configuration();
		
		// Input and output paths for each job
		Path inputPath = new Path(args[0]);
		Path wcInputPath = inputPath;
		Path wcOutputPath = new Path("output/WordCount");
		Path dsInputPath = wcOutputPath;
		Path dsOutputPath = new Path("output/DocSize");
		Path tfidfInputPath = dsOutputPath;
		Path tfidfOutputPath = new Path("output/TFIDF");
		
		// Get/set the number of documents (to be used in the TFIDF MapReduce job)
        FileSystem fs = inputPath.getFileSystem(conf);
        FileStatus[] stat = fs.listStatus(inputPath);
		String numDocs = String.valueOf(stat.length);
		conf.set("numDocs", numDocs);
		
		// Delete output paths if they exist
		FileSystem hdfs = FileSystem.get(conf);
		if (hdfs.exists(wcOutputPath))
			hdfs.delete(wcOutputPath, true);
		if (hdfs.exists(dsOutputPath))
			hdfs.delete(dsOutputPath, true);
		if (hdfs.exists(tfidfOutputPath))
			hdfs.delete(tfidfOutputPath, true);
		
		// Create and execute Word Count job
		
			/************ YOUR CODE HERE ************/

		Job j=new Job(conf,"WordCountWithMR");
		j.setJarByClass(TFIDF.class);
		j.setMapperClass(WCMapper.class);
		j.setReducerClass(WCReducer.class);
		j.setOutputKeyClass(Text.class);
		j.setOutputValueClass(IntWritable.class);
		FileInputFormat.addInputPath(j, inputPath);
		FileOutputFormat.setOutputPath(j, wcOutputPath);

		
			
		// Create and execute Document Size job
		
			/************ YOUR CODE HERE ************/
		/*Configuration conf2 = new Configuration();
		FileSystem fs2 = inputPath.getFileSystem(conf2);
        FileStatus[] stat2 = fs2.listStatus(dsInputPath);
		String numDocs2 = String.valueOf(stat2.length);
		conf2.set("numDocs2", numDocs2);*/
		j.waitForCompletion(true);
		Job j2=new Job(conf,"TFWithMR");
		j2.setJarByClass(TFIDF.class);
		j2.setMapperClass(DSMapper.class);
		j2.setReducerClass(DSReducer.class);
		j2.setOutputKeyClass(Text.class);
		j2.setOutputValueClass(Text.class);
		FileInputFormat.addInputPath(j2, dsInputPath);
		FileOutputFormat.setOutputPath(j2, dsOutputPath);

		//Create and execute TFIDF job
		
			/************ YOUR CODE HERE ************/
		
		j2.waitForCompletion(true);
		Job j3=new Job(conf,"TFWithMR");
		j3.setJarByClass(TFIDF.class);
		j3.setMapperClass(TFIDFMapper.class);
		j3.setReducerClass(TFIDFReducer.class);
		j3.setOutputKeyClass(Text.class);
		j3.setOutputValueClass(Text.class);
		FileInputFormat.addInputPath(j3, tfidfInputPath);
		FileOutputFormat.setOutputPath(j3, tfidfOutputPath);
		j3.waitForCompletion(true);
		//System.exit(j2.waitForCompletion(true)?0:1);
    }
	
	/*
	 * Creates a (key,value) pair for every word in the document 
	 *
	 * Input:  ( byte offset , contents of one line )
	 * Output: ( (word@document) , 1 )
	 *
	 * word = an individual word in the document
	 * document = the filename of the document
	 */
	public static class WCMapper extends Mapper<Object, Text, Text, IntWritable> {
		
		/************ YOUR CODE HERE ************/
		public void map(Object key, Text value, Context con) throws IOException, InterruptedException
		{
			String line = value.toString();
			String docName = ((FileSplit) con.getInputSplit()).getPath().getName();
			String[] words=line.split(" ");
			for(String word: words )
			{
			  Text outputKey = new Text(word.trim()+'@'+docName);
			  IntWritable outputValue = new IntWritable(1);
			  con.write(outputKey, outputValue);
			}
		}
    }

    /*
	 * For each identical key (word@document), reduces the values (1) into a sum (wordCount)
	 *
	 * Input:  ( (word@document) , 1 )
	 * Output: ( (word@document) , wordCount )
	 *
	 * wordCount = number of times word appears in document
	 */
	public static class WCReducer extends Reducer<Text, IntWritable, Text, IntWritable> {
		
		/************ YOUR CODE HERE ************/
		public void reduce(Text word, Iterable<IntWritable> values, Context con) throws IOException, InterruptedException
		{
		int sum = 0;
		   for(IntWritable value : values)
		   {
		   sum += value.get();
		   }
		   con.write(word, new IntWritable(sum));
		}

    }
	
	/*
	 * Rearranges the (key,value) pairs to have only the document as the key
	 *
	 * Input:  ( (word@document) , wordCount )
	 * Output: ( document , (word=wordCount) )
	 */
	public static class DSMapper extends Mapper<Object, Text, Text, Text> {
		
		/************ YOUR CODE HERE ************/
		//int docCount = new File("/mnt/sdcard/folder").listFiles().length;
		public void map(Object key, Text inpText, Context con) throws IOException, InterruptedException
		{
			String inpString = inpText.toString();
			//String docName = ((FileSplit) con.getInputSplit()).getPath().getName();
			String[] keys=inpString.split("\\t");
			if(keys.length==2){
				String[] words = keys[0].split("\\@");
				Text outputKey = new Text(words[1]);
				Text outputValue = new Text(words[0]+"="+keys[1]);
				con.write(outputKey, outputValue);
			}
			
			/*for(String word: words )
			{
			  Text outputKey = new Text(word.trim()+'@'+docName);
			  IntWritable outputValue = new IntWritable(1);
			  con.write(outputKey, outputValue);
			}*/
		}
    }

    /*
	 * For each identical key (document), reduces the values (word=wordCount) into a sum (docSize) 
	 *
	 * Input:  ( document , (word=wordCount) )
	 * Output: ( (word@document) , (wordCount/docSize) )
	 *
	 * docSize = total number of words in the document
	 */
	public static class DSReducer extends Reducer<Text, Text, Text, Text> {
		
		/************ YOUR CODE HERE ************/
		public void reduce(Text documentKey, Iterable<Text> values, Context con) throws IOException, InterruptedException
		{
			int docSize = 0;
			List<String> inpVals = new ArrayList<String>();
			for(Text value : values)
			{
				inpVals.add(value.toString());
				//System.out.println(value);
				String[] vals = value.toString().split("=");
				docSize += Integer.parseInt(vals[1]);
			}

			for(String value : inpVals)
			{
				String[] vals = value.split("=");
				int wordCount = Integer.parseInt(vals[1]);
				Text outputKey = new Text(vals[0]+'@'+documentKey);
				Text outputValue = new Text( (new Integer(wordCount)).toString()+'/'+(new Integer(docSize)).toString());
				//System.out.println(outputKey+","+outputValue);
				con.write(outputKey, outputValue);
			}
		}
    }
	
	/*
	 * Rearranges the (key,value) pairs to have only the word as the key
	 * 
	 * Input:  ( (word@document) , (wordCount/docSize) )
	 * Output: ( word , (document=wordCount/docSize) )
	 */
	public static class TFIDFMapper extends Mapper<Object, Text, Text, Text> {

		/************ YOUR CODE HERE ************/
		public void map(Object key, Text inpText, Context con) throws IOException, InterruptedException
		{
			String inpString = inpText.toString();
			//String docName = ((FileSplit) con.getInputSplit()).getPath().getName();
			String[] keys=inpString.split("\\t");
			if(keys.length==2){
				String[] words = keys[0].split("\\@");
				Text outputKey = new Text(words[0]);
				Text outputValue = new Text(words[1]+"="+keys[1]);
				con.write(outputKey, outputValue);
			}
		}
    }

    /*
	 * For each identical key (word), reduces the values (document=wordCount/docSize) into a 
	 * the final TFIDF value (TFIDF). Along the way, calculates the total number of documents and 
	 * the number of documents that contain the word.
	 * 
	 * Input:  ( word , (document=wordCount/docSize) )
	 * Output: ( (document@word) , TFIDF )
	 *
	 * numDocs = total number of documents
	 * numDocsWithWord = number of documents containing word
	 * TFIDF = (wordCount/docSize) * ln(numDocs/numDocsWithWord)
	 *
	 * Note: The output (key,value) pairs are sorted using TreeMap ONLY for grading purposes. For
	 *       extremely large datasets, having a for loop iterate through all the (key,value) pairs 
	 *       is highly inefficient!
	 */
	public static class TFIDFReducer extends Reducer<Text, Text, Text, Text> {
		
		private static int numDocs;
		private Map<Text, Text> tfidfMap = new HashMap<>();
		
		// gets the numDocs value and stores it
		protected void setup(Context context) throws IOException, InterruptedException {
			Configuration conf = context.getConfiguration();
			numDocs = Integer.parseInt(conf.get("numDocs"));
		}
		
		public void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
			
			/************ YOUR CODE HERE ************/
			int numDocsWithWord = 0;
	 		List<String> inpVals = new ArrayList<String>();
	 		for(Text value:values){
	 			inpVals.add(value.toString());
				//System.out.println(value);
				numDocsWithWord++;
	 		}
	 		for(String value : inpVals)
			{
				String[] vals = value.split("[=/]");
				Text outputKey = new Text(vals[0]+'@'+key);
				int numerator = Integer.parseInt(vals[1]);
				int denominator = Integer.parseInt(vals[2]);
				double tfidfValue = Math.log((double)numDocs/numDocsWithWord)*((double)numerator/denominator);
				Text outputValue = new Text( String.valueOf(tfidfValue) );
				//Put the output (key,value) pair into the tfidfMap instead of doing a context.write
				tfidfMap.put(outputKey,outputValue);
				/*int wordCount = Integer.parseInt(vals[1]);
				Text outputKey = new Text(vals[0]+'@'+documentKey);
				Text outputValue = new Text( (new Integer(wordCount)).toString()+'/'+(new Integer(docSize)).toString());
				System.out.println(outputKey+","+outputValue);
				con.write(outputKey, outputValue);*/
			}

			//Put the output (key,value) pair into the tfidfMap instead of doing a context.write
			//tfidfMap.put(/*document@word*/, /*TFIDF*/);
		}
		
		// sorts the output (key,value) pairs that are contained in the tfidfMap
		protected void cleanup(Context context) throws IOException, InterruptedException {
            Map<Text, Text> sortedMap = new TreeMap<Text, Text>(tfidfMap);
			for (Text key : sortedMap.keySet()) {
                context.write(key, sortedMap.get(key));
            }
        }
		
		
    }
}
